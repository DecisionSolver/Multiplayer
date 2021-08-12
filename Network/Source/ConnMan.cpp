#include "pch.h"
#include "ConnMan.h"
#include <system_error>

#include "File System/File_system.h"

std::shared_ptr<File_system> FS;

#include "File System/Project System/Project.h"
extern std::unique_ptr<ProjectFile> Project;

std::map<asio::ip::udp::endpoint, Connection::SharedPtr> ConnectionManager::m_connectionsUDP;
std::map<asio::ip::tcp::endpoint, Connection::SharedPtr> ConnectionManager::m_connectionsTCP;

std::mutex m_connectionsMutex;

//------------------------------------------------------------------------------
ConnectionManager::ConnectionManager(TypeWorking _Type, TypeProtocol _Proto, std::string IP, USHORT port,
	size_t numThreads): m_io_service(), m_acceptor(asio::ip::tcp::acceptor(m_io_service,
		asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(IP), port)))
	, m_threads(numThreads)
	, _IP(IP)
	, _Port(port)
	, _Type(_Type)
	, _Proto(_Proto)
#if defined(USE_SSL)
	, Context_SSL(asio::ssl::context::sslv23)
#endif
{
	m_SocketUDP.reset(new asio::ip::udp::socket(m_io_service,
		_Type == TypeWorking::Server ?
		asio::ip::udp::endpoint(asio::ip::address_v4::from_string(IP), port)
		: asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)));
	m_SocketUDP->set_option(asio::ip::udp::socket::reuse_address(true));
}

//------------------------------------------------------------------------------
ConnectionManager::~ConnectionManager()
{
	if (IsWorking)
		StopSystem();
}

//------------------------------------------------------------------------------
void ConnectionManager::StartSystem(std::function<void(Connection::SharedPtr)> Func)
{
	if (_Type == TypeWorking::Client && one_connection && one_connection->getIsError())
		return;

	if (_Type == TypeWorking::Server)
	{
		FS = std::make_shared<File_system>();
		//Project->Open("New");
	}

#if defined(USE_SSL)
	if (_Type != TypeWorking::Client && 
		(!IsSetupPathsCert_All || !(IsSetupPathsCert_Chain && IsSetupPathsCert_Private && IsSetupPathsCert_DH)))
	{
#if __has_include("logger.h")
		Logger_Error("Paths to certificates was not set up yet, before 'Start' function you need to call all the following:"\
			"Set_Cert_Chain, Set_Private_Key, Set_TMP_DH\nOr only one: Set_All_Paths!");
#endif
		return;
	}

	Context_SSL.set_options(
		asio::ssl::context::default_workarounds
		| asio::ssl::context::no_sslv2
		| asio::ssl::context::single_dh_use);
	//Context_SSL.set_password_callback(
	//	[&]()
	//{
	//	return "test";
	//});
	if (_Type != TypeWorking::Client)
	{
		asio::error_code ec;
		if (IsSetupPathsCert_RSA_Private_Key)
		{
			Context_SSL.use_rsa_private_key_file(SSL_RSA_Private_Key/*"keys/rootca.key"*/,
				asio::ssl::context_base::file_format::pem, ec);
			if (ec)
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Process With RSA Private Key! Error Message: %s\nError ID: %i",
					ec.message().c_str(), ec.value());
#endif
		}
		Context_SSL.use_certificate_chain_file(SSL_Cert_Chain/*"keys/rootca.crt"*/, ec);
		if (ec)
#if __has_include("logger.h")
			Logger_Error_F("Unseccessful Process With Chain File! Error Message: %s\nError ID: %i",
				ec.message().c_str(), ec.value());
#endif
		Context_SSL.use_private_key_file(SSL_Private_Key/*"keys/rootca.key"*/,
			asio::ssl::context_base::file_format::pem, ec);
		if (ec)
#if __has_include("logger.h")
			Logger_Error_F("Unseccessful Process With Private Key File! Error Message: %s\nError ID: %i",
				ec.message().c_str(), ec.value());
#endif
		Context_SSL.use_tmp_dh_file(SSL_TMP_DH/*"keys/dh2048.pem"*/, ec);
		if (ec)
#if __has_include("logger.h")
			Logger_Error_F("Unseccessful Process With D-H File! Error Message: %s\nError ID: %i",
				ec.message().c_str(), ec.value());
#endif
	}
#endif

	if (m_io_service.stopped())
		m_io_service.reset();

	const char *UseSSL =
#if defined(USE_SSL)
		"YES"
#else
		"NO"
#endif
		;

#if __has_include("logger.h")
	Logger_Warn_F("The Listening IP: %s\nPort: %lu\nProtocol: %sIs Secured By SSL? %s", _IP.c_str(), _Port,
		_Proto == TypeProtocol::TCP ? "TCP\n" : "UDP\n", UseSSL);
#endif

	DoAccept();

	// Log that we are starting the io_service thread
	for (auto &thread: m_threads)
	{
		if (!thread.joinable())
			thread = std::thread(&ConnectionManager::IoServiceThreadProc, this);
	}

	IsWorking = true;

	Curr = Last = std::chrono::high_resolution_clock::now();
	Handler(Func ? Func : nullptr);
}

//------------------------------------------------------------------------------
void ConnectionManager::StopSystem()
{
	IsWorking = false;

	if (_Type == TypeWorking::Server && _Proto == TypeProtocol::TCP)
		m_connectionsTCP.clear();
	else if (_Type == TypeWorking::Server && _Proto == TypeProtocol::UDP)
		m_connectionsUDP.clear();

	if (_Type == TypeWorking::Client && one_connection && (one_connection->GetStopped()
		|| one_connection->IsConnected() ||
		one_connection->GetLogged()))
	{
		if (!one_connection->getIsError())
			one_connection->Stop();
		one_connection.reset();
	}
	m_io_service.stop();

	if (_Type == TypeWorking::Server && MySQL_DB)
	{
		MySQL_DB->Disconnect();
		MySQL_DB.reset();
	}

	for (auto &thread: m_threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

bool ConnectionManager::ConnectToServer()
{
	if (_Type == TypeWorking::Server || (one_connection && (one_connection->IsConnected()
		|| one_connection->GetLogged())))
	{
		return false;
	}

#if __has_include("logger.h")
	Logger_Info_F("Trying To Connect To IP: %s And Port: %lu Server!", _IP.c_str(), _Port);
#endif

	asio::error_code ec;

	// Create the connection from the connected socket
	if (_Proto == TypeProtocol::TCP)
	{
#if defined(USE_SSL)
		newConnTCP_SSL.reset(new asio::ssl::stream<asio::ip::tcp::socket>(m_io_service, Context_SSL));
#else
		m_SocketTCP.reset(new asio::ip::tcp::socket(m_io_service));
		m_SocketTCP->connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(_IP), _Port), ec);
#endif

		one_connection = Connection::Create(this,
#if defined(USE_SSL)
			std::move(newConnTCP_SSL)
#else
				*m_SocketTCP
#endif
			);

#if defined(USE_SSL)
		one_connection->get_socketTCP().set_verify_mode(asio::ssl::verify_peer);
		one_connection->get_socketTCP().set_verify_callback(ConnectionManager::verify_certificate);

		one_connection->get_socketTCP().lowest_layer().connect(tcp::endpoint(asio::ip::address::from_string(_IP), _Port), ec);
#endif
	}
	else
		one_connection = Connection::Create(this);

#if defined(USE_SSL)
	if (!ec || (one_connection && one_connection->get_socketTCP().lowest_layer().is_open())
#else
	if (!ec || (m_SocketTCP && m_SocketTCP->is_open())

#endif
		|| m_SocketUDP)
	{
#if defined(USE_SSL)
		if (!IsSetupPathsCert_Chain)
		{
#if __has_include("logger.h")
			Logger_Error("Paths to certificates was not set up yet, before 'Start' "\
				"or at least 'ConnectoToServer' function you need to call "\
				"Set_Cert_Chain!");
#endif
			return false;
		}

		Context_SSL.load_verify_file(SSL_Cert_Chain/*"keys/rootca.crt"*/, ec);
		if (ec)
#if __has_include("logger.h")
			Logger_Error_F("Unseccessful Process With Verify File! Error Message: %s\nError ID: %i",
				ec.message().c_str(), ec.value());
#endif
		if (IsSetupPathsCert_RSA_Private_Key)
		{
			Context_SSL.use_rsa_private_key_file(SSL_RSA_Private_Key/*"keys/rootca.key"*/,
				asio::ssl::context_base::file_format::pem, ec);
			if (ec)
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Process With RSA Private Key! Error Message: %s\nError ID: %i",
					ec.message().c_str(), ec.value());
#endif
		}

		one_connection->get_socketTCP().handshake(asio::ssl::stream_base::client, ec);
		if (ec)
		{
#if __has_include("logger.h")
			Logger_Error_F("Unseccessful Handshake! Error Message: %s\nError ID: %i", ec.message().c_str(), ec.value());
#endif
			return false;
		}
#endif

#if __has_include("logger.h")
		Logger_Info("Success Connecting! Now Sending Back-Response");
#endif
		if (_Proto == TypeProtocol::UDP)
		{
			asio::ip::udp::resolver resolver(m_io_service);
			asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(asio::ip::udp::v4(),
				_IP, std::to_string(_Port));
			one_connection->SetEndPoint(receiver_endpoint);
		}
	
		one_connection->Start();

		/* Sending ACCEPT CONNECTION Packet */
		std::shared_ptr<network::Packet> AnswerPacket = std::make_shared<network::Packet>();
		AnswerPacket->CreatePacket(network::Packet::Type::Connection, true, { {"_1", "OK"} });

		one_connection->Send(AnswerPacket);
		
		return true;
	}
	else
	{
#if __has_include("logger.h")
		Logger_Error_F("Failed Connecting! Error: %s\nAbort Connecting!",
			Connection::ErrorCodeToString(ec).str().c_str());
#endif
		one_connection->getIsError() = true;
		one_connection->get_error_queue().push_back(ec ? ec : asio::error::operation_aborted);
		one_connection->get_cv_error().notify_one();

		StopSystem();
		return false;
	}
}

bool ConnectionManager::IsRunning() const
{
	return IsWorking;
}

void ConnectionManager::Send(const std::string &Packet)
{
	if (_Type == ConnectionManager::TypeWorking::Client)
	{
		if (one_connection)
			one_connection->Send({ Packet.begin(), Packet.end() });
	}
	else
	{
		if (_Proto == TypeProtocol::TCP)
		{
#if defined(USE_SSL)
			for (auto &connection: m_connectionsTCP)
#else
			for (auto connection: m_connectionsTCP)
#endif
			{
				connection.second->Send({ Packet.begin(), Packet.end() });
			}
		}
		else if (_Proto == TypeProtocol::UDP)
		{
			for (auto connection: m_connectionsUDP)
			{
				connection.second->Send({ Packet.begin(), Packet.end() });
			}
		}
	}
}
void ConnectionManager::Send(const std::shared_ptr<network::Packet> &Packet)
{
	if (_Type == ConnectionManager::TypeWorking::Client)
	{
		if (one_connection)
			one_connection->Send(Packet);
	}
	else
	{
		if (_Proto == TypeProtocol::TCP)
		{
#if defined(USE_SSL)
			for (auto &connection: m_connectionsTCP)
#else
			for (auto connection: m_connectionsTCP)
#endif
			{
				connection.second->Send(Packet);
			}
		}
		else if (_Proto == TypeProtocol::UDP)
		{
			for (auto connection: m_connectionsUDP)
			{
				connection.second->Send(Packet);
			}
		}
	}
}

void ConnectionManager::SetCB_Accept(std::function<void(Connection::SharedPtr)> Func)
{
	Callback_Accept = Func;
}
void ConnectionManager::SetCB_OnPacketHandle(std::function<void(Connection::SharedPtr)> Func)
{
	Callback_OnClientHandler = Func;
}
void ConnectionManager::SetCB_OnLoggin(std::function<void(Connection::SharedPtr)> Func)
{
	Callback_OnLoggin = Func;
}
void ConnectionManager::SetCB_OnError(std::function<void(asio::error_code)> Func)
{
	Callback_OnError = Func;
}

std::condition_variable cvBlocking;
std::mutex muxBlocking;
std::atomic_bool NoMessageLeft = true;

//------------------------------------------------------------------------------
void ConnectionManager::IoServiceThreadProc()
{
	try
	{
		// Run the asynchronous callbacks from the socket on this thread
		// Until the io_service is stopped from another thread
		m_io_service.run();
	}
	catch (std::system_error &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("System error caught in io_service socket thread. Exception: %s\nError Code: %d\n",
			e.what(), e.code().value());
#endif
	}
	catch (std::exception &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Standard exception caught in io_service socket thread. Exception: %s\n", e.what());
#endif
	}
	catch (...)
	{
#if __has_include("logger.h")
		Logger_Error_F("Unhandled exception caught in io_service socket thread.\n");
#endif
	}

	WaitForMySQL.notify_all();
#if __has_include("logger.h")
	Logger_Info_F("%s",
		_Type == TypeWorking::Client ?
		"Trying Stopping Listening Now!\n" : "The Server Trying Stopping Now!\n");
#endif
	IsWorking = false;

	NoMessageLeft = false;
	cvBlocking.notify_one();
}

// This is a server connection (to not to do another methods and other things)
// It seems just like a one user but it's a sever worker xD
Connection::SharedPtr OneConnForServerUDP;

//------------------------------------------------------------------------------
void ConnectionManager::DoAccept()
{
	if (_Type == TypeWorking::Client) return;

	if (_Proto == TypeProtocol::TCP)
	{
#if defined(USE_SSL)
		newConnTCP_SSL.reset(new asio::ssl::stream<asio::ip::tcp::socket>(m_io_service, Context_SSL));
#else
		newConnTCP.reset(new asio::ip::tcp::socket(m_io_service));
#endif
	}
	else
	{
		OneConnForServerUDP = Connection::Create(this);
		if (Callback_Accept)
			Callback_Accept(OneConnForServerUDP);
		
		asio::ip::udp::resolver resolver(m_io_service);
		asio::ip::udp::resolver::query query(asio::ip::udp::v4(), _IP, std::to_string(_Port));
		asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);
		OneConnForServerUDP->SetEndPoint(receiver_endpoint);
		OneConnForServerUDP->Start();
	}

	if (_Proto == TypeProtocol::TCP)
	{
#if defined(USE_SSL)
		m_acceptor.async_accept(newConnTCP_SSL->lowest_layer(),
#else
		m_acceptor.async_accept(*newConnTCP,
#endif
			[&](const asio::error_code errorCode)
		{
			if (errorCode)
			{
#if __has_include("logger.h")
				Logger_Error_F("An error occured while attemping to accept connections. Error Code: %s\n",
					Connection::ErrorCodeToString(errorCode).str().c_str());
#endif
			}

#if defined(USE_SSL)
			asio::error_code ec;
			newConnTCP_SSL->handshake(asio::ssl::stream_base::server, ec);
			if (ec)
			{
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Handshake! Error Message: %s\nError ID: %i", ec.message().c_str(), ec.value());
#endif
				return;
				//connectionTCP->get_socketTCP().lowest_layer().close();
			}
#endif
#if defined(USE_SSL)
			Connection::SharedPtr connectionTCP = Connection::Create(this, std::move(newConnTCP_SSL));
#else
			Connection::SharedPtr connectionTCP = Connection::Create(this, *newConnTCP);
#endif

#if __has_include("logger.h")
			std::string IP =
#if defined(USE_SSL)
				connectionTCP->get_socketTCP().lowest_layer().remote_endpoint().address().to_string();
#else
				connectionTCP->get_socketTCP().remote_endpoint().address().to_string();
#endif
			USHORT Port =
#if defined(USE_SSL)
				connectionTCP->get_socketTCP().lowest_layer().remote_endpoint().port();
#else
				connectionTCP->get_socketTCP().remote_endpoint().port();
#endif

			Logger_Info_F("Accept New Client! Where Are You Frommmmm? IP: %s, Port: %u", IP.c_str(), Port);
#endif

#if defined(USE_SSL)
			connectionTCP->get_socketTCP().handshake(asio::ssl::stream_base::server, ec);
			if (ec)
			{
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Handshake! Error Message: %s\nError ID: %i", ec.message().c_str(), ec.value());
#endif
				return;
				//connectionTCP->get_socketTCP().lowest_layer().close();
			}
#endif
			{
				std::scoped_lock<std::mutex> lock(m_connectionsMutex);

				// Create the connection from the connected socket
#if defined(USE_SSL)
				auto itConnection = std::find_if(m_connectionsTCP.begin(), m_connectionsTCP.end(),
					[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
#else
				auto itConnection = std::find_if(m_connectionsTCP.begin(), m_connectionsTCP.end(),
					[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
#endif
				{
					if (ThisConn.second == connectionTCP)
						return true;
					return false;
				});
#if defined(USE_SSL)
				if (itConnection == m_connectionsTCP.end())
					m_connectionsTCP[connectionTCP->get_socketTCP().lowest_layer().remote_endpoint()] = connectionTCP;
				else
					m_connectionsTCP.erase(itConnection);
#else
				if (itConnection == m_connectionsTCP.end())
					m_connectionsTCP[connectionTCP->get_socketTCP().remote_endpoint()] = connectionTCP;
				else
					m_connectionsTCP.erase(itConnection);
#endif
			}

			if (Callback_Accept)
				Callback_Accept(connectionTCP);

			connectionTCP->Start();

			DoAccept();
		});
	}
}

//------------------------------------------------------------------------------
void ConnectionManager::OnConnectionClosed(Connection::SharedPtr connection, std::function<void()> Func)
{
	if (_Type == TypeWorking::Client)
	{
#if __has_include("logger.h")
		Logger_Info("Disconnected Current Client!");
#endif
		connection->successConn.notify_all();

		if (connection->getIsError() &&
			!connection->get_error_queue().empty())
			connection->get_error_queue().pop_front();

		if (connection)
			connection.reset();
		return;
	}

#if __has_include("logger.h")
	Logger_Info("Disconnected Some Client!");
#endif
	if (_Type == TypeWorking::Server && _Proto == TypeProtocol::TCP)
	{
		auto itConnection = std::find_if(m_connectionsTCP.begin(), m_connectionsTCP.end(),
			[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
		{
			if (ThisConn.second == connection)
				return true;
			return false;
		});
		if (itConnection != m_connectionsTCP.end())
		{
			if (itConnection->second->IsConnected() && itConnection->second->GetLogged()
				&& !itConnection->second->getIsError())
				MySQL_DB->UpdateValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
					std::to_string(connection->GetMetaDB_User()) + "'" } });

#if defined(USE_SSL)
			ToDo("Very Insecure!");
			//one_connection->get_socketTCP().shutdown();
#else
			if (m_SocketTCP)
			{
				m_SocketTCP->close();
				m_SocketTCP.reset();
			}
#endif
			m_connectionsTCP.erase(itConnection);

			connection->Stop(false);

			if (Func)
				Func();
			}
		}
	else
		one_connection.reset();
}

Connection::SharedPtr ConnectionManager::GetConnect()
{
	//std::scoped_lock<std::mutex> lock(m_connectionsMutex);

	if (_Type == ConnectionManager::TypeWorking::Server) return nullptr;

	if (one_connection)
		return one_connection;

	return Connection::SharedPtr();
}

std::condition_variable &ConnectionManager::IsWait()
{
	if (_Type == TypeWorking::Client && one_connection)
		return one_connection->successConn;
}

//------------------------------------------------------------------------------
void ConnectionManager::Handler(std::function<void(Connection::SharedPtr)> Func)
{
	if (!m_io_service.stopped() && !isUpdate)
	{
		if (Func)
			Callback_OnClientHandler = Func;
		
		std::thread([&]
		{
			auto Lambd = [&](Connection::SharedPtr connection)
			{
				network::Packet packet = network::Packet();
				if (_Type == TypeWorking::Server)
				{
					connection->GetPacket(packet, network::Packet::Type::Disconnection);
					if (packet)
					{
						OnConnectionClosed(connection);
						return false;
					}
				}

				if (connection && (!connection->GetLogged()))
				{
					if (_Type == TypeWorking::Server)
					{
						connection->GetPacket(packet, network::Packet::Type::MySQL);
						if (packet)
						{
							json temp = packet.getData();
							if (!temp.empty())
							{
								std::string Login = temp["_0"].get<std::string>(),
									Pass = temp["_1"].get<std::string>();

								auto Obj = MySQL_DB->SelectValues("Local", { "*" },
									{ " WHERE _0 = '" + Login + "' AND _1 = '" + Pass + "'" });

								// If Successful Then Send Answer About It
								std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
								json pack = Answer->CreatePacket(network::Packet::Type::MySQL)->getData();
								if (!Obj.empty())
								{
									pack["data"]["body"]["_0"] = "OK";

									connection->SetMetaDB_User((int)Obj["_N"].front().get<json::number_integer_t>());

									if (Obj["_2"].front().get<json::number_integer_t>() == 1)
										pack["data"]["body"]["_0"] = "AlreadyOnl";
								}
								else
									pack["data"]["body"]["_0"] = "NotFound";
								Answer->FillIn(pack);

								connection->Send(Answer);

								if (pack["data"]["body"]["_0"] == "NotFound" ||
									pack["data"]["body"]["_0"] == "AlreadyOnl")
								{
									OnConnectionClosed(connection);
									return false;
								}
								if (pack["data"]["body"]["_0"] == "OK")
								{
									connection->SetLogged();
									MySQL_DB->UpdateValues("Local", { "_2" }, { { "1" } }, { {
										" WHERE _N = '" +
										std::to_string(connection->GetMetaDB_User()) + "'" } });

									return true;
								}
							}
							else
							{
								OnConnectionClosed(connection);
								return false;
							}
						}
					}

					connection->GetPacket(packet, network::Packet::Type::Connection);
					if (packet)
					{
						json dataJSON;
						if (_Type == TypeWorking::Client)
						{
							Logger_Warn_F("We're get the network::Packet::Type::Connection and try to check 'OK',"\
								" from thi data: %s", packet.getData().dump().c_str());
							dataJSON = packet.getData();
							if (!dataJSON.empty() && dataJSON["_1"] == "OK")
							{
								connection->SetConnected(true);
								connection->successConn.notify_all();
								if (Callback_OnLoggin)
									Callback_OnLoggin(connection);
							}
						}
						else
						{
							/* Sending ACCEPT CONNECTION Packet */
							std::shared_ptr<network::Packet> AnswerPacket = std::make_shared<network::Packet>();
							AnswerPacket->CreatePacket(network::Packet::Type::Connection, true,
								{ {"_1", "OK"} });
							connection->Send(AnswerPacket);
							connection->SetConnected(true);
						}
					}

					// If Wasn't MySQL Packet And Timer Is Done
					if (_Type == TypeWorking::Server && connection->GetTimer())
					{
						OnConnectionClosed(connection);
						return false;
					}
				}

				if (Callback_OnClientHandler && connection && connection->IsConnected())
					Callback_OnClientHandler(connection);

				return true;
			};
			if (_Type == TypeWorking::Server)
			{
				{
					std::unique_lock<std::mutex> MySQL_Lock(m_MySQL);
					WaitForMySQL.wait(MySQL_Lock);
				}
			}
			while ((!m_io_service.stopped() || IsRunning()) && !isUpdate)
			{
				while ((_Type == TypeWorking::Server &&
					(_Proto == TypeProtocol::TCP ? m_connectionsTCP.empty() : m_connectionsUDP.empty()))
					|| NoMessageLeft)
				{
					std::unique_lock<std::mutex> ul(muxBlocking);
					cvBlocking.wait(ul);
				}

				if (_Type == TypeWorking::Client && one_connection)
				{
					while (one_connection && (one_connection->getIsError() &&
						!one_connection->get_error_queue().empty()))
					{
						Sleep(1000);
						if (Callback_OnError)
						{
							Callback_OnError(one_connection->get_error_queue().back());
							return;
						}
					}
					Lambd(one_connection);
				}
				if (_Type == TypeWorking::Server)
				{
					if (_Proto == TypeProtocol::TCP)
					{
						{
							std::unique_lock<std::mutex> MainLock(m_connectionsMutex);
							std::map<asio::ip::tcp::endpoint, Connection::SharedPtr>::iterator connection =
								m_connectionsTCP.begin();
							while (connection != m_connectionsTCP.end())
							{
								if (!connection->second) continue;

								if (!connection->second->GetStopped() && (!connection->second->getIsError() &&
									connection->second->get_error_queue().empty()))
								{
									if (!Lambd(connection->second))
									{
										connection = m_connectionsTCP.begin();
										if (connection == m_connectionsTCP.end())
											connection = m_connectionsTCP.begin();
										continue;
									}
								}
								else if (connection->second->getIsError() &&
									!connection->second->get_error_queue().empty())
								{
									OnConnectionClosed(connection->second);
									
									connection = m_connectionsTCP.begin();
									if (connection == m_connectionsTCP.end())
										connection = m_connectionsTCP.begin();
									continue;
								}
								connection++;
							}
						}
					}
					else if (_Proto == TypeProtocol::UDP)
					{
						{
							std::unique_lock<std::mutex> MainLock(m_connectionsMutex);
							std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator connection =
								m_connectionsUDP.begin();
							while (connection != m_connectionsUDP.end())
							{
								if (!connection->second) continue;

								if (!connection->second->GetStopped())
								{
									if (!Lambd(connection->second))
									{
										connection = m_connectionsUDP.begin();
										if (connection == m_connectionsUDP.end())
											connection = m_connectionsUDP.begin();
										continue;
									}
								}

								connection++;
							}
						}
					}
				}
			}
		}).detach();
	}
}

#if defined(USE_SSL)
bool ConnectionManager::verify_certificate(bool preverified, asio::ssl::verify_context &ctx)
{
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. For example, RFC 2818 describes
	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	// documentation for more details. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.

	// In this example we will simply print the certificate's subject name.
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	std::cout << "Verifying " << subject_name << "\n";

	return preverified;
}

#endif
