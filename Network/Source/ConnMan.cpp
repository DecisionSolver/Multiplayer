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
ConnectionManager::ConnectionManager(const TypeWorking &_Type, const TypeProtocol &_Proto,
	const std::string &IP, const USHORT &port, size_t numThreads):
	m_io_service()
	, m_threads(numThreads)
	, _IP(IP)
	, _Port(port)
	, _Type(_Type)
	, _Proto(_Proto)
#if defined(USE_SSL)
	, Context_SSL(asio::ssl::context::sslv23)
#endif
{
}

//------------------------------------------------------------------------------
ConnectionManager::~ConnectionManager()
{
	if (IsWorking)
		StopSystem();
}

bool ConnectionManager::LoadConfig()
{
	if (_Type == TypeWorking::Server)
	{
		if (!FS)
			FS = std::make_shared<File_system>();

		auto Data = FS->LoadSettingsFile();

		// MySQL

		if (_Proto == TypeProtocol::FTP)
		{
			_IP = Data.get<std::string>("Server FTP.ip", "127.0.0.1");
			_Port = Data.get<USHORT>("Server FTP.port", 21);
			std::string ip = Data.get<std::string>("Server MySQL FTP.ip", "127.0.0.1");
			//USHORT port = Data.get<USHORT>("Server MySQL FTP.port", 3306);
			std::string db = Data.get<std::string>("Server MySQL FTP.db", "");
			std::string login = Data.get<std::string>("Server MySQL FTP.login", "");
			std::string pass = Data.get<std::string>("Server MySQL FTP.pass", "");
			std::string table = Data.get<std::string>("Server MySQL FTP.table", "");

			FTPServer = std::make_shared<fineftp::FtpServer>(_IP, _Port, FS->SetPathFTP(),
				login, pass, ip, db, table);
		}
		else
		{
			std::string ip = Data.get<std::string>("Server MySQL.ip", "127.0.0.1");
			USHORT port = Data.get<USHORT>("Server MySQL.port", 3306);

			std::string db = Data.get<std::string>("Server MySQL.db", "");
			std::string login = Data.get<std::string>("Server MySQL.login", "");
			std::string pass = Data.get<std::string>("Server MySQL.pass", "");
			std::string table = Data.get<std::string>("Server MySQL.table", "");

			if (MySQL_DB->Connect(login, pass, ip, db, table, port) == mysql::Client::Done)
			{
#if __has_include("logger.h")
				Logger_Debug_F("[MYSQL] Successful Connected To {}", ip);
#endif
#if __has_include("logger.h")
				Logger_Info("[MYSQL] Successful Connect To MySQL DB");
#endif
			}
			else
			{
#if __has_include("logger.h")
				Logger_Debug_F("[MYSQL] Failure Connected To {}", ip);
#endif
#if __has_include("logger.h")
				Logger_Info("[MYSQL] Failure Connect To MySQL DB");
#endif
				return false;
			}

			_IP = Data.get<std::string>("Server.ip", "127.0.0.1");
			_Port = Data.get<USHORT>("Server.port", 25565);
			
			if (_Proto == TypeProtocol::TCP)
			{
				m_acceptor = std::make_shared<asio::ip::tcp::acceptor>(
					asio::ip::tcp::acceptor(asio::ip::tcp::acceptor(m_io_service,
					asio::ip::tcp::endpoint(asio::ip::address::from_string(_IP), _Port))));
				m_acceptor->set_option(asio::ip::tcp::socket::reuse_address(true));
			}

			{
				std::string Proto = Data.get<std::string>("Server.protocol", "tcp");
				boost::to_lower(Proto);
				if (Proto != "tcp" && Proto != "udp")
					_Proto = TypeProtocol::TCP;
				else
					_Proto = (Proto == "tcp" ? TypeProtocol::TCP : TypeProtocol::UDP);
			}

#if defined(USE_SSL)
			if (!IsSetupPathsCert_All || !(IsSetupPathsCert_Chain && IsSetupPathsCert_Private && IsSetupPathsCert_DH))
			{
				Set_Cert_Chain(Data.get<std::string>("Server.Cert_Chain", ""));
				Set_Private_Key(Data.get<std::string>("Server.Cerf_Private_Key", ""));
				Set_TMP_DH(Data.get<std::string>("Server.TMP_DH", ""));
				Set_Cert_RSA_Private(Data.get<std::string>("Server.RSA_Private_Key", ""));
			}
#endif
		}
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
void ConnectionManager::StartSystem(const std::function<void(Connection::SharedPtr)> &Func)
{
	if (_Type == TypeWorking::Server)
	{
		if (_Proto == TypeProtocol::FTP)
		{
			LoadConfig();

			if (FTPServer && FTPServer->start(m_threads.size()))
			{
#if __has_include("logger.h")
				Logger_Info_F("FTP Server Has Been Started On IP {} And Port 2121 And 4 Threads", _IP);
#endif
			}
			else
			{
#if __has_include("logger.h")
				Logger_Critical("Something Is Wrong With Starting FTP Server!");
#endif
			}
			IsWorking = true;

			const char *UseSSL =
#if defined(USE_SSL)
				"YES"
#else
				"NO"
#endif
				;

#if __has_include("logger.h")
			Logger_Warn_F("{} {}\nPort: {}\nProtocol: {} Is Secured By SSL? {}", "Listening IP:",
				_IP, _Port, "FTP\n", UseSSL);
#endif

			return;
		}
		else if (!LoadConfig())
		{
#if __has_include("logger.h")
			Logger_Critical("[SERVER] Something Is Wrong With Read Settings From File!");
#endif
		}

#if defined(USE_SSL)
		if (!IsSetupPathsCert_All || !(IsSetupPathsCert_Chain && IsSetupPathsCert_Private && IsSetupPathsCert_DH))
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
					Logger_Error_F("Unseccessful Process With RSA Private Key! Error Message: {}\nError ID: {}",
						ec.message(), ec.value());
#endif
			}
			Context_SSL.use_certificate_chain_file(SSL_Cert_Chain/*"keys/rootca.crt"*/, ec);
			if (ec)
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Process With Chain File! Error Message: {}\nError ID: {}",
					ec.message(), ec.value());
#endif
			Context_SSL.use_private_key_file(SSL_Private_Key/*"keys/rootca.key"*/,
				asio::ssl::context_base::file_format::pem, ec);
			if (ec)
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Process With Private Key File! Error Message: {}\nError ID: {}",
					ec.message(), ec.value());
#endif
			Context_SSL.use_tmp_dh_file(SSL_TMP_DH/*"keys/dh2048.pem"*/, ec);
			if (ec)
#if __has_include("logger.h")
				Logger_Error_F("Unseccessful Process With D-H File! Error Message: {}\nError ID: {}",
					ec.message(), ec.value());
#endif
		}
#endif
	}

	if (_Type == TypeWorking::Client && one_connection)
	{
		if (one_connection->getIsError())
			return;

		if (one_connection->get_socketTCP()->is_open())
			one_connection->Start();
	}

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
	Logger_Warn_F("{} {}\nPort: {}\nProtocol: {} Is Secured By SSL? {}",
		_Type == TypeWorking::Server ? "Listening IP:" : "Connected To IP:",
		_IP, _Port,
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

	Handler(Func ? Func : nullptr);
}

//------------------------------------------------------------------------------
void ConnectionManager::StopSystem()
{
	IsWorking = false;
	if (_Type == TypeWorking::Server)
	{
#if !defined(USE_SSL)
		if (_Proto == TypeProtocol::TCP)
			m_connectionsTCP.clear();
#endif
		if (_Proto == TypeProtocol::UDP)
			m_connectionsUDP.clear();

		if (MySQL_DB)
		{
			MySQL_DB.reset();
		}
	}
	if (_Type == TypeWorking::Client && one_connection && (one_connection->GetStopped()
		|| one_connection->IsConnected() ||
		one_connection->GetLogged()))
	{
		if (!one_connection->getIsError())
			one_connection->Stop();
		one_connection.reset();
	}

	m_io_service.stop();

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
		return false;

#if __has_include("logger.h")
	Logger_Info_F("Trying To Connect To IP: {} And Port: {} Server!", _IP, _Port);
#endif

	asio::error_code ec;
	/* Sending ACCEPT CONNECTION Packet */
	std::shared_ptr<network::Packet> AnswerPacket = std::make_shared<network::Packet>();
	AnswerPacket->CreatePacket(network::Packet::Type::Connection, true, { { "_1", "OK" } });

	// Create the connection from the connected socket
	if (_Proto == TypeProtocol::TCP)
	{
#if defined(USE_SSL)
		newConnTCP_SSL.reset(new asio::ssl::stream<asio::ip::tcp::socket>(m_io_service, Context_SSL));
#else
		one_connection = std::make_shared<Connection>(this, m_io_service);
		one_connection->get_socketTCP()->connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(_IP), _Port), ec);
#endif

#if defined(USE_SSL)
		one_connection->get_socketTCP().set_verify_mode(asio::ssl::verify_peer);
		one_connection->get_socketTCP().set_verify_callback(ConnectionManager::verify_certificate);

		one_connection->get_socketTCP().lowest_layer().connect(tcp::endpoint(asio::ip::address::from_string(_IP), _Port), ec);
#endif
	}
	else if (_Proto == TypeProtocol::UDP)
	{
		auto EndPoint = asio::ip::udp::endpoint(asio::ip::address::from_string(_IP), _Port);
		one_connection = std::make_shared<Connection>(this, m_io_service, EndPoint);
		one_connection->get_socketUDP()->open(asio::ip::udp::v4(), ec);
		one_connection->SetEndPoint(EndPoint);
		one_connection->get_socketUDP()->send_to(asio::buffer(AnswerPacket->getData().dump()), EndPoint);
	}

#if defined(USE_SSL)
	if (!ec || (one_connection && one_connection->get_socketTCP().lowest_layer().is_open())
#else
	if (!ec && (_Proto == TypeProtocol::TCP && one_connection->get_socketTCP()->is_open())

#endif
	 || (_Proto == TypeProtocol::UDP && one_connection->get_socketUDP()->is_open()))
	{
#if defined(USE_SSL)
		if (!IsSetupPathsCert_Chain)
		{
#if __has_include("logger.h")
			Logger_Error("[CLIENT] Paths to certificates was not set up yet, before 'Start' "\
				"or at least 'ConnectoToServer' function you need to call "\
				"Set_Cert_Chain!");
#endif
			return false;
		}

		Context_SSL.load_verify_file(SSL_Cert_Chain/*"keys/rootca.crt"*/, ec);
		if (ec)
#if __has_include("logger.h")
			Logger_Error_F("[CLIENT] Unseccessful Process With Verify File! Error Message: {}\nError ID: {}",
				ec.message(), ec.value());
#endif
		if (IsSetupPathsCert_RSA_Private_Key)
		{
			Context_SSL.use_rsa_private_key_file(SSL_RSA_Private_Key/*"keys/rootca.key"*/,
				asio::ssl::context_base::file_format::pem, ec);
			if (ec)
#if __has_include("logger.h")
				Logger_Error_F("[CLIENT] Unseccessful Process With RSA Private Key! Error Message: {}\nError ID: {}",
					ec.message(), ec.value());
#endif
		}

		one_connection->get_socketTCP().handshake(asio::ssl::stream_base::client, ec);
		if (ec)
		{
#if __has_include("logger.h")
			Logger_Error_F("[CLIENT] Unseccessful Handshake! Error Message: {}\nError ID: {}", ec.message(), ec.value());
#endif
			return false;
		}
#endif

#if __has_include("logger.h")
		Logger_Info("[CLIENT] Success Connecting! Now Sending Back-Response");
#endif

		if (_Proto == TypeProtocol::TCP)
			one_connection->Send(AnswerPacket);

		return true;
	}
	else
	{
#if __has_include("logger.h")
		Logger_Error_F("[CLIENT] Failed Connecting! Error: {}\nAbort Connecting!",
			Connection::ErrorCodeToString(ec).str());
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
		if (_Proto == TypeProtocol::UDP)
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
		if (_Proto == TypeProtocol::UDP)
		{
			for (auto connection: m_connectionsUDP)
			{
				connection.second->Send(Packet);
			}
		}
	}
}

void ConnectionManager::SetCB_Accept(const std::function<void(Connection::SharedPtr)> &Func)
{
	Callback_Accept = Func;
}
void ConnectionManager::SetCB_OnPacketHandle(const std::function<void(Connection::SharedPtr)> &Func)
{
	Callback_OnClientHandler = Func;
}
void ConnectionManager::SetCB_OnLoggin(const std::function<void(Connection::SharedPtr)> &Func)
{
	Callback_OnLoggin = Func;
}
void ConnectionManager::SetCB_OnError(const std::function<void(asio::error_code)> &Func)
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
		DWORD last_error = ::GetLastError();
		asio::error_code ec(last_error, asio::error::get_system_category());
		asio::detail::throw_error(ec, "IoServiceThreadProc");
	}
	catch (std::system_error &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("System error caught in io_service socket thread. Exception: {}\nError Code: {}\n",
			e.what(), e.code().value());
#endif
	}
	catch (std::exception &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Standard exception caught in io_service socket thread. Exception: {}\n", e.what());
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
	Logger_Info_F("{}",
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
#if defined(USE_SSL)
		m_acceptor->async_accept(newConnTCP_SSL->lowest_layer(),
#else
		m_acceptor->async_accept(*newConnTCP,
#endif
		[&](const asio::error_code errorCode)
		{
			if (errorCode)
			{
#if __has_include("logger.h")
				Logger_Error_F("An error occured while attemping to accept connections. Error Code: {}\n",
					Connection::ErrorCodeToString(errorCode).str());
#endif
			}

#if defined(USE_SSL)
			asio::error_code ec;
			newConnTCP_SSL->handshake(asio::ssl::stream_base::server, ec);
			if (ec)
			{
#if __has_include("logger.h")
				Logger_Error_F("[SERVER] Unseccessful Handshake! Error Message: {}\nError ID: {}", ec.message(), ec.value());
#endif
				return;
				//connectionTCP->get_socketTCP().lowest_layer().close();
			}
#endif

			Curr = Last = std::chrono::high_resolution_clock::now();

#if defined(USE_SSL)
			Connection::SharedPtr connectionTCP = std::make_shared<Connection>(this, std::move(newConnTCP_SSL));
#else
			Connection::SharedPtr connectionTCP = std::make_shared<Connection>(this, m_io_service, std::move(newConnTCP));
#endif

#if __has_include("logger.h")
			std::string IP =
#if defined(USE_SSL)
				connectionTCP->get_socketTCP().lowest_layer().remote_endpoint().address().to_string();
#else
				connectionTCP->get_socketTCP()->remote_endpoint().address().to_string();
#endif
			USHORT Port =
#if defined(USE_SSL)
				connectionTCP->get_socketTCP().lowest_layer().remote_endpoint().port();
#else
				connectionTCP->get_socketTCP()->remote_endpoint().port();
#endif

			Logger_Info_F("[SERVER] Accept New Client! Where Are You Frommmmm? IP: {}, Port: {}", IP, Port);
#endif

#if defined(USE_SSL)
			connectionTCP->get_socketTCP().handshake(asio::ssl::stream_base::server, ec);
			if (ec)
			{
#if __has_include("logger.h")
				Logger_Error_F("[SERVER] Unseccessful Handshake! Error Message: {}\nError ID: {}", ec.message(), ec.value());
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
					m_connectionsTCP[connectionTCP->get_socketTCP()->remote_endpoint()] = connectionTCP;
				else
					m_connectionsTCP.erase(itConnection);
#endif
			}

			if (Callback_Accept)
				Callback_Accept(connectionTCP);

			connectionTCP->Start();
			Curr = std::chrono::high_resolution_clock::now();

			DoAccept();
		});
	}
	else
	{
		auto EndPoint = asio::ip::udp::endpoint(asio::ip::address::from_string(_IP), _Port);
		OneConnForServerUDP = std::make_shared<Connection>(this, m_io_service, EndPoint);
		Curr = Last = std::chrono::high_resolution_clock::now();
		if (Callback_Accept)
			Callback_Accept(OneConnForServerUDP);

		OneConnForServerUDP->Start();
		Curr = std::chrono::high_resolution_clock::now();
	}
}

//------------------------------------------------------------------------------
void ConnectionManager::OnConnectionClosed(Connection::SharedPtr connection, bool Need2DiscFromMySQL)
{
	std::lock_guard<std::mutex> lk(m_ConnectedClose);
	if (_Type == TypeWorking::Client)
	{
#if __has_include("logger.h")
		Logger_Info("[CLIENT] Disconnect Client!");
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
	Logger_Info("[SERVER] Disconnect Client!");
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

		// Check If Still Connected
		if (itConnection != m_connectionsTCP.end())
		{
			if (Need2DiscFromMySQL)
				MySQL_DB->UpdateValues(std::vector<std::string>{ "_2" }, { { "0" } }, { { " WHERE _N = '" +
					std::to_string(connection->GetMetaDB_User()) + "'" } });

#if defined(USE_SSL)
			ToDo("Very Insecure!");
			connection->get_socketTCP().shutdown();
#else
			connection->get_socketTCP()->close();
#endif
			m_connectionsTCP.erase(itConnection);

			connection->Stop(false);
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

bool ConnectionManager::GetTimer(const Connection::SharedPtr &User)
{
	if (User->GetLogged() ||
		(User->m_owner && User->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client)) return false;

	Last = std::chrono::high_resolution_clock::now();
	auto Diff = (Last - Curr);

#if __has_include("logger.h")
	Logger_Info_F("[SERVER] Client({}) Has Time: {}\n", User->m_clientId,
		std::chrono::duration_cast<std::chrono::seconds>(Diff).count());
#endif

	if (Diff > std::chrono::seconds(60))
		return true;

	return false;
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
				if (!connection) return false;

				network::Packet packet = network::Packet();
				if (_Type == TypeWorking::Server)
				{
					connection->GetPacket(packet, network::Packet::Type::Disconnection);
					if (packet)
					{
						OnConnectionClosed(connection);
						return false;

						packet.clear();
					}
				}

				connection->GetPacket(packet, network::Packet::Type::Connection);
				if (packet)
				{
					json dataJSON;
					if (_Type == TypeWorking::Client)
					{
						Logger_Warn_F("[CLIENT] We're get the network::Packet::Type::Connection and try to check 'OK',"\
							" from this data: {}", packet.getData().dump());
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
					packet.clear();
				}

				if (_Type == TypeWorking::Client)
				{
					// Clear The Login Packet That Signal About Good Or Fail Log in (Came From Server)
					connection->GetPacket(packet, network::Packet::Type::Login, "OK");
					if (packet)
						packet.clear();
				}
				if (_Type == TypeWorking::Server && !connection->GetLogged())
				{
					connection->GetPacket(packet, network::Packet::Type::Login);
					if (packet)
					{
						json temp = packet.getData();
						if (!temp.empty())
						{
							std::string Login = temp["_0"].get<std::string>(),
								Pass = temp["_1"].get<std::string>();

							auto Obj = MySQL_DB->SelectValues(std::vector<std::string>{ "*" },
								{ " WHERE _0 = '" + Login + "' AND _1 = '" + Pass + "'" });

							// If Successful Then Send Answer About It
							std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
							json pack = Answer->CreatePacket(network::Packet::Type::Login)->getData();
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
								OnConnectionClosed(connection, false);
								return false;
							}
							if (pack["data"]["body"]["_0"] == "OK")
							{
								connection->SetLogged();
								MySQL_DB->UpdateValues(std::vector<std::string>{ "_2" }, { { "1" } }, { {
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
						packet.clear();
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
				std::thread([&]
				{
					while ((!m_io_service.stopped() || IsRunning()) && !isUpdate)
					{
						std::this_thread::sleep_for(1s);
						if (_Proto == TypeProtocol::TCP && m_connectionsTCP.empty() ||
							_Proto == TypeProtocol::UDP && m_connectionsUDP.empty()) continue;

						std::unique_lock<std::mutex> MainLock(m_connectionsMutex);
						if (_Proto == TypeProtocol::TCP)
						{
							std::map<asio::ip::tcp::endpoint, Connection::SharedPtr>::iterator connection =
								m_connectionsTCP.begin();
							while (connection != m_connectionsTCP.end())
							{
								if (!connection->second) continue;

								if (!connection->second->GetStopped() && (!connection->second->getIsError() &&
									connection->second->get_error_queue().empty()))
								{
									// If Wasn't MySQL Packet And Timer Is Done
									if (connection->second->IsConnected())
									{
										if (GetTimer(connection->second))
										{
											Logger_Info("[SERVER] User Has Disconnected By Left Time Waiting For MySQL Packet!");

											std::shared_ptr<network::Packet> AnswerPacket = std::make_shared<network::Packet>();
											AnswerPacket->CreatePacket(network::Packet::Type::Disconnection, true, { { "_0",
												"[SERVER] User Has Been Disconnected By Left Time Waiting For MySQL Packet!" } });

											connection->second->Send(AnswerPacket);
											OnConnectionClosed(connection->second);

											connection = m_connectionsTCP.begin();
											if (connection == m_connectionsTCP.end())
												connection = m_connectionsTCP.begin();
											continue;
										}
										// Check If We Connected In MySQL
										else if (connection->second->GetLogged())
										{
											auto Obj = MySQL_DB->SelectValues(std::vector<std::string>{ "_2" }, { { " WHERE _N = '" +
													std::to_string(connection->second->GetMetaDB_User()) + "'" } });
											if (!Obj.empty())
											{
												if (Obj["_0"] == 0)
												{
													OnConnectionClosed(connection->second);

													connection = m_connectionsTCP.begin();
													if (connection == m_connectionsTCP.end())
														connection = m_connectionsTCP.begin();
													continue;
												}
											}
										}
									}
								}
								connection++;
							}
						}
						if (_Proto == TypeProtocol::UDP)
						{
							std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator connection =
								m_connectionsUDP.begin();
							while (connection != m_connectionsUDP.end())
							{
								if (!connection->second) continue;

								if (!connection->second->GetStopped() && (!connection->second->getIsError() &&
									connection->second->get_error_queue().empty()))
								{
									// If Wasn't MySQL Packet And Timer Is Done
									if (connection->second->IsConnected())
									{
										if (GetTimer(connection->second))
										{
											Logger_Info("[SERVER] User Has Disconnected By Left Time Waiting For MySQL Packet!");
											OnConnectionClosed(connection->second);
											connection = m_connectionsUDP.begin();
											if (connection == m_connectionsUDP.end())
												connection = m_connectionsUDP.begin();
											continue;
										}
										// Check If We Connected In MySQL
										else if (connection->second->GetLogged())
										{
											auto Obj = MySQL_DB->SelectValues(std::vector<std::string>{ "_2" }, { { " WHERE _N = '" +
													std::to_string(connection->second->GetMetaDB_User()) + "'" } });
											if (!Obj.empty())
											{
												if (Obj["_0"] == 0)
												{
													OnConnectionClosed(connection->second);

													connection = m_connectionsUDP.begin();
													if (connection == m_connectionsUDP.end())
														connection = m_connectionsUDP.begin();
													continue;
												}
											}
										}
									}
								}
								connection++;
							}
						}
					}
				}).detach();
			}
			while ((!m_io_service.stopped() || IsRunning()) && !isUpdate)
			{
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
					if (_Proto == TypeProtocol::UDP)
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
				while ((_Type == TypeWorking::Server &&
					(_Proto == TypeProtocol::TCP ?
						m_connectionsTCP.empty() :
						m_connectionsUDP.empty())
					)
					|| NoMessageLeft)
				{
					std::unique_lock<std::mutex> ul(muxBlocking);
					cvBlocking.wait(ul);
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
	
#if __has_include("logger.h")
	Logger_Info_F("Verifying {}", subject_name);
#endif

	return preverified;
}

#endif

void ConnectionManager::SendPacket(bool ExceptionClient, const Connection::SharedPtr &connection, const std::shared_ptr<network::Packet> &Packet)
{
#if !defined(USE_SSL)
	if (_Proto == TypeProtocol::TCP)
	{
		for (const auto &Next: m_connectionsTCP)
		{
			// Not To ME!!!
			if (ExceptionClient && (connection && connection == Next.second)) continue;
			Next.second->Send(Packet);
		}
	}
#endif 
	if (_Proto == TypeProtocol::UDP)
	{
		for (const auto &Next: m_connectionsUDP)
		{
			// Not To ME!!!
			if (ExceptionClient && (connection && connection == Next.second)) continue;
			Next.second->Send(Packet);
		}
	}
}
