#include "pch.h"
#include "ConnMan.h"
#include <system_error>

using asio::ip::tcp;

//------------------------------------------------------------------------------
ConnectionManager::ConnectionManager(TypeWorking _Type, std::string IP, UINT port, size_t numThreads):
	m_io_service()
	, m_acceptor(m_io_service, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(IP), (USHORT)port))
	, m_threads(numThreads)
	, _IP(IP)
	, _Port(port)
	, _Type(_Type)
{
	m_Socket.reset(new asio::ip::tcp::socket(m_io_service));
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

	if (m_io_service.stopped())
		m_io_service.reset();

	DoAccept();

	for (auto &thread: m_threads)
	{
		if (!thread.joinable())
			thread = std::thread(&ConnectionManager::IoServiceThreadProc, this);
	}

	IsWorking = true;

	if (Func)
		Handler(Func);
	else
		Handler(nullptr);
}

//------------------------------------------------------------------------------
void ConnectionManager::StopSystem()
{
	std::scoped_lock<std::mutex> lock(m_stop_sys);

	IsWorking = false;

	m_connections.clear();

	if (_Type == TypeWorking::Client && one_connection && (one_connection->get_stopped() || one_connection->IsConnected()))
	{
		one_connection->Stop();
		one_connection.reset();
	}
	// TODO - Will the stopping of the io_service be enough to kill all the connections and
	//		  ultimately have them get destroyed?
	//        Because remember they have outstanding ref count to thier shared_ptr in the async handlers
	m_io_service.stop();

	if (User)
	{
		User->Disconnect();
		User.reset();
	}

	for (auto &thread: m_threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

#include <boost/lambda/lambda.hpp>
using boost::lambda::var;
using boost::lambda::_1; 
void ConnectionManager::ConnectToServer()
{
	if (_Type == TypeWorking::Server || (one_connection && one_connection->IsConnected()))
		return;

	Sleep(500);

	// Set up the variable that receives the result of the asynchronous
	// operation. The error code is set to would_block to signal that the
	// operation is incomplete. Asio guarantees that its asynchronous
	// operations will never fail with would_block, so any other value in
	// ec indicates completion.
	asio::error_code ec = asio::error::would_block;

	// Start the asynchronous operation itself. The boost::lambda function
	// object is used as a callback and will update the ec variable when the
	// operation completes. The blocking_udp_client.cpp example shows how you
	// can use boost::bind rather than boost::lambda.

	m_Socket.reset(new asio::ip::tcp::socket(m_io_service));
	m_Socket->connect(tcp::endpoint(asio::ip::address::from_string(_IP), (USHORT)_Port), ec);

	// Determine whether a connection was successfully established. The
	// deadline actor may have had a chance to run and close our socket, even
	// though the connect operation notionally succeeded. Therefore we must
	// check whether the socket is still open before deciding if we succeeded
	// or failed.
	
	// Create the connection from the connected socket
	one_connection = Connection::Create(this, *m_Socket);
	if (!ec || m_Socket->is_open())
	{
		one_connection->Start();
	
		/* Sending ACCEPT CONNECTION Packet */
		swl::Packet AnswerPacket = swl::Packet();
		json dataJSON = AnswerPacket.CreateMessage();
		dataJSON["data"]["body"]["_1"] = "OK";
		AnswerPacket.FillIn(swl::Packet::Header(swl::Packet::Type::Connection), dataJSON);
		one_connection->Send(AnswerPacket);
		//throw asio::system_error(ec ? ec : asio::error::operation_aborted);
	}
	else
	{
		one_connection->getIsError() = true;
		one_connection->get_error_queue().push_back(ec ? ec : asio::error::operation_aborted);
		one_connection->get_cv_error().notify_one();

		StopSystem();
	}
}

bool ConnectionManager::IsRunning() const
{
	return IsWorking;
}

void ConnectionManager::Send(std::string Packet)
{
	if (_Type == ConnectionManager::TypeWorking::Client) return;

	for (auto connection: m_connections)
	{
		connection->Send({ Packet.begin(), Packet.end() });
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

//------------------------------------------------------------------------------
void ConnectionManager::IoServiceThreadProc()
{
	try
	{
		// Log that we are starting the io_service thread
		printf("io_service socket thread starting.\n");

		// Run the asynchronous callbacks from the socket on this thread
		// Until the io_service is stopped from another thread
		m_io_service.run();
	}
	catch (std::system_error &e)
	{
		printf("System error caught in io_service socket thread. Error Code: %d\n", e.code().value());
	}
	catch (std::exception &e)
	{
		printf("Standard exception caught in io_service socket thread. Exception: %s\n", e.what());
	}
	catch (...)
	{
		printf("Unhandled exception caught in io_service socket thread.\n");
	}

	printf("io_service socket thread exiting.\n");
}

//------------------------------------------------------------------------------
void ConnectionManager::DoAccept()
{
	if (_Type == TypeWorking::Client) return;

	newConn.reset(new asio::ip::tcp::socket(m_io_service));
	m_acceptor.async_accept(*newConn,
		[this](const asio::error_code errorCode)
	{
		if (errorCode)
		{
			printf("An error occured while attemping to accept connections. Error Code: %s\n",
				Connection::ErrorCodeToString(errorCode).str().c_str());
			return;
		}

		std::scoped_lock<std::mutex> lock(m_do_accept);
		// Create the connection from the connected socket
		Connection::SharedPtr connection = Connection::Create(this, *newConn);
		
#if !defined(_DEBUG)
		if (connection->get_socket().local_endpoint().address().to_string() == "127.0.0.1")
			connection->SetApproved();
#endif

		m_connections.push_back(connection);

		if (Callback_Accept)
			Callback_Accept(m_connections.back());
		connection->Start();

		DoAccept();
	});
}

//------------------------------------------------------------------------------
void ConnectionManager::OnConnectionClosed(Connection::SharedPtr connection)
{
	std::scoped_lock<std::mutex> lock(m_onconn_close);
	
	if (_Type == TypeWorking::Client)
	{
		connection->successConn.notify_all();
		if (connection)
			connection.reset();

		return;
	}

	if (_Type == TypeWorking::Server)
	{
		auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
		if (itConnection != m_connections.end())
		{
			User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
				std::to_string(connection->GetMetaDB_User()) + "'" } });

			if (m_Socket)
				m_Socket.reset();
			m_connections.erase(itConnection);
		}
	}
	else
		one_connection.reset();
}

std::vector<Connection::SharedPtr> ConnectionManager::GetAllConnections()
{
	std::scoped_lock<std::mutex> lock(m_get_allconn);
	if (!m_connections.empty())
	{
		if (_Type == TypeWorking::Server)
			return m_connections;
		if (_Type == TypeWorking::Client)
			return { m_connections.back() };
	}

	return {};
}

Connection::SharedPtr ConnectionManager::GetConnect()
{
	std::scoped_lock<std::mutex> lock(m_get_allconn);
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
			auto Lambd = [&](Connection::SharedPtr &connection)
			{
				if (!connection)
				{
					if (_Type == TypeWorking::Server)
					{
						auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
						if (itConnection != m_connections.end())
							m_connections.erase(itConnection);
					}
					else
						one_connection.reset();

					return;
				}

				swl::Packet packet = swl::Packet();
				if (_Type == TypeWorking::Server)
				{
					connection->GetPacket(packet, swl::Packet::Type::Disconnection);
					if (packet)
					{
						auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
						if (itConnection != m_connections.end())
						{
							User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
								std::to_string(connection->GetMetaDB_User()) + "'" } });

							m_connections.erase(itConnection);
						}

						return;
					}

					// Updating FTP Server
					connection->GetPacket(packet, swl::Packet::Type::ClosedServerByUpdate);
					if (packet && connection->get_socket().local_endpoint().address().to_string() == "127.0.0.1")
					{
						swl::Packet Answer = swl::Packet();
						json pack = Answer.CreateAnswer();
						Answer.FillIn(swl::Packet::Header((swl::Packet::Type)
							(swl::Packet::Type::Answer << swl::Packet::Type::ClosedServerByUpdate)), pack);
						connection->Send(Answer);
						
						isUpdate = true;
						auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
						if (itConnection != m_connections.end())
							m_connections.erase(itConnection);
						return;
					}
				}

				if (connection && (!connection->GetApproved()))
				{
					if (_Type == TypeWorking::Server)
					{
						connection->GetPacket(packet, swl::Packet::Type::MySQL);
						if (packet)
						{
							json temp = json::parse(packet.getData());
							if (!temp.empty())
							{
								std::string Login = temp["_0"].get<std::string>(),
									Pass = temp["_1"].get<std::string>();

								auto Obj = User->TrySelectValues("Local", { "*" },
									{ " WHERE _0 = '" + Login + "' AND _1 = '" + Pass + "'" });

								//printf(("\nsize: " + std::to_string(Obj.size()) + "\n").c_str());

								// If Successfull Then Send Answer About It
								swl::Packet Answer = swl::Packet();
								json pack = Answer.CreateAnswer();
								if (!Obj.empty() && !Obj.front().second.empty())
								{
									pack["data"]["body"]["_0"] = "OK";

									connection->SetMetaDB_User((int)Obj.back().second["_N"].get<json::value_t>());

									if ((int)Obj.back().second["_2"].get<json::value_t>() == 1)
										pack["data"]["body"]["_0"] = "AlreadyOnl";
								}
								else
									pack["data"]["body"]["_0"] = "NotFound";

								Answer.FillIn(swl::Packet::Header((swl::Packet::Type)
									(swl::Packet::Type::Answer << swl::Packet::Type::MySQL)), pack);
								connection->Send(Answer);

								if (pack["data"]["body"]["_0"] == "NotFound" ||
									pack["data"]["body"]["_0"] == "AlreadyOnl")
								{
									auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
									if (itConnection != m_connections.end())
										m_connections.erase(itConnection);

									return;
								}
								if (pack["data"]["body"]["_0"] == "OK")
								{
									connection->SetApproved();
									connection->SetConnected(true);
									User->TryInsertValues("Local", { "_2" }, { { "1" } }, { {
										" WHERE _N = '" + std::to_string(connection->GetMetaDB_User()) + "'" } });

									return;
								}
							}
							else
							{
								auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
								if (itConnection != m_connections.end())
								{
									User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
										std::to_string(connection->GetMetaDB_User()) + "'" } });
									m_connections.erase(itConnection);
								}

								return;
							}
						}
					}

					connection->GetPacket(packet, swl::Packet::Type::Connection);
					if (packet)
					{
						json dataJSON;
						if (_Type == TypeWorking::Client)
						{
							dataJSON = json::parse(packet.getData());
							if (!dataJSON.empty() && dataJSON["_1"] == "OK")
							{
								connection->SetConnected(true);
								connection->successConn.notify_all();
								Callback_OnLoggin(connection);
							}
						}
						else
						{
							/* Sending ACCEPT CONNECTION Packet */
							swl::Packet AnswerPacket = swl::Packet();
							dataJSON["data"]["body"]["_1"] = "OK";
							AnswerPacket.FillIn(swl::Packet::Header(swl::Packet::Type::Connection), dataJSON);
							connection->Send(AnswerPacket);
						}
					}

					if (_Type == TypeWorking::Server && connection->GetTimer())
					{
						auto itConnection = std::find(m_connections.begin(), m_connections.end(), connection);
						if (itConnection != m_connections.end())
						{
							User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
								std::to_string(connection->GetMetaDB_User()) + "'" } });
							m_connections.erase(itConnection);
						}

						return;
					}
				}

				if (Callback_OnClientHandler && connection && connection->IsConnected())
					Callback_OnClientHandler(connection);
			};

			while ((!m_io_service.stopped() || IsRunning()) && !isUpdate)
			{
				if (_Type == TypeWorking::Server)
					Sleep(500);

				if (_Type == TypeWorking::Client && one_connection)
				{
					std::scoped_lock<std::mutex> lock(m_main_handler);
					while (one_connection && (one_connection->getIsError() && !one_connection->get_error_queue().empty()))
					{
						if (Callback_OnError)
						{
							std::mutex New;
							std::unique_lock<std::mutex> OnErrorLock(New);

							Sleep(1000);
							Callback_OnError(one_connection->get_error_queue().back());
							return;
						}
					}
					Lambd(one_connection);

					if (one_connection)
						one_connection->waiterDisconnection.notify_all();
				}

				if (_Type == TypeWorking::Server)
				{
					for (auto &connection: m_connections)
					{
						std::unique_lock<std::mutex> lock(m_main_handler);
						if (!connection) continue;

						if (!connection->get_stopped())
							Lambd(connection);
					
						if (connection)
							connection->waiterDisconnection.notify_all();
					}
				}
			}
		}).detach();
	}
}
