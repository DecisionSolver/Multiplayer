#include "pch.h"
#include "ConnMan.h"
#include <system_error>

std::map<asio::ip::udp::endpoint, Connection::SharedPtr> ConnectionManager::m_connectionsUDP;
std::map<asio::ip::tcp::endpoint, Connection::SharedPtr> ConnectionManager::m_connectionsTCP;

std::mutex m_connectionsMutex;

//------------------------------------------------------------------------------
ConnectionManager::ConnectionManager(TypeWorking _Type, TypeProtocol _Proto, std::string IP, UINT port, size_t numThreads) :
	m_io_service()
	, m_acceptor(asio::ip::tcp::acceptor(m_io_service, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(IP),
	(USHORT)port)))
	, m_threads(numThreads)
	, _IP(IP)
	, _Port(port)
	, _Type(_Type)
	, _Proto(_Proto)
{
	if (_Proto == TypeProtocol::TCP)
	{
		//auto NewSock = new asio::ip::tcp::socket(m_io_service,
		//	asio::ip::tcp::endpoint(asio::ip::address_v4::from_string(IP), (USHORT)port));
		//NewSock->set_option(asio::ip::tcp::socket::reuse_address(true));
		//m_SocketTCP.reset(NewSock);
	}
	else
	{
		m_SocketUDP.reset(new asio::ip::udp::socket(m_io_service,
			_Type == TypeWorking::Server ?
			asio::ip::udp::endpoint(asio::ip::address_v4::from_string(IP), (USHORT)port)
			: asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)));
		m_SocketUDP->set_option(asio::ip::udp::socket::reuse_address(true));
	}
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

	Curr = Last = std::chrono::high_resolution_clock::now();
	if (Func)
		Handler(Func);
	else
		Handler(nullptr);
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

	if (_Type == TypeWorking::Server && User)
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

bool ConnectionManager::ConnectToServer()
{
	if (_Type == TypeWorking::Server || (one_connection && (one_connection->IsConnected()
		|| one_connection->GetLogged())))
		return false;

	asio::error_code ec;

	if (_Proto == TypeProtocol::TCP)
	{
		m_SocketTCP.reset(new asio::ip::tcp::socket(m_io_service));
		m_SocketTCP->connect(tcp::endpoint(asio::ip::address::from_string(_IP), (USHORT)_Port), ec);
	}
	
	// Create the connection from the connected socket
	if (_Proto == TypeProtocol::TCP)
		one_connection = Connection::Create(this, *m_SocketTCP);
	else
		one_connection = Connection::Create(this);

	if (!ec || (m_SocketTCP && m_SocketTCP->is_open()) || m_SocketUDP)
	{
		if (_Proto == TypeProtocol::UDP)
		{
			asio::ip::udp::resolver resolver(m_io_service);
			asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(asio::ip::udp::v4(), _IP, std::to_string(_Port));
			one_connection->SetEndPoint(receiver_endpoint);
		}
	
		one_connection->Start();

		/* Sending ACCEPT CONNECTION Packet */
		swl::Packet AnswerPacket = swl::Packet();
		json dataJSON = json::parse(AnswerPacket.CreateMessage()->getData());
		dataJSON["data"]["body"]["_1"] = "OK";
		AnswerPacket.FillIn(swl::Packet::Header(swl::Packet::Type::Connection), dataJSON);
		one_connection->Send(AnswerPacket);
		
		return true;
	}
	else
	{
		one_connection->getIsError() = true;
		one_connection->get_error_queue().push_back(ec ? ec : asio::error::operation_aborted);
		one_connection->get_cv_error().notify_one();

		StopSystem();
		return false;
	}

	return false;
}

bool ConnectionManager::IsRunning() const
{
	return IsWorking;
}

void ConnectionManager::Send(std::string Packet)
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
			for (auto connection: m_connectionsTCP)
			{
				connection.second->Send({ Packet.begin(), Packet.end() });
			}
		}
		else if (_Proto == TypeProtocol::UDP)
		{
			for (auto connection: m_connectionsTCP)
			{
				connection.second->Send({ Packet.begin(), Packet.end() });
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

	WaitForMySQL.notify_all();
	printf("io_service socket thread exiting.\n");
}

// This is a server connection (to not to do another methods and other things)
// It seems just like a one user but it's a sever worker xD
Connection::SharedPtr OneConnForServerUDP;

//------------------------------------------------------------------------------
void ConnectionManager::DoAccept()
{
	if (_Type == TypeWorking::Client) return;

	if (_Proto == TypeProtocol::TCP)
		newConnTCP.reset(new asio::ip::tcp::socket(m_io_service));
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
		m_acceptor.async_accept(*newConnTCP,
			[this](const asio::error_code errorCode)
		{
			if (errorCode)
			{
				printf("An error occured while attemping to accept connections. Error Code: %s\n",
					Connection::ErrorCodeToString(errorCode).str().c_str());
				return;
			}

			std::scoped_lock<std::mutex> lock(m_connectionsMutex);
			// Create the connection from the connected socket
			Connection::SharedPtr connectionTCP = Connection::Create(this, *newConnTCP);

			auto itConnection = std::find_if(m_connectionsTCP.begin(), m_connectionsTCP.end(),
				[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
			{
				if (ThisConn.second == connectionTCP)
					return true;
				return false;
			});
			if (itConnection == m_connectionsTCP.end())
				m_connectionsTCP[connectionTCP->get_socketTCP().remote_endpoint()] = connectionTCP;
			else
				m_connectionsTCP.erase(itConnection);

			if (Callback_Accept)
				Callback_Accept(connectionTCP);
			connectionTCP->Start();

			DoAccept();
		});
	}
}

//------------------------------------------------------------------------------
void ConnectionManager::OnConnectionClosed(Connection::SharedPtr connection)
{
	std::scoped_lock<std::mutex> lock(m_connectionsMutex);
	
	if (_Type == TypeWorking::Client)
	{
		connection->successConn.notify_all();
		if (connection)
			connection.reset();

		return;
	}

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
			User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
				std::to_string(connection->GetMetaDB_User()) + "'" } });

			if (m_SocketTCP)
				m_SocketTCP.reset();
			m_connectionsTCP.erase(itConnection);
		}
	}
	else
		one_connection.reset();
}

Connection::SharedPtr ConnectionManager::GetConnect()
{
	std::scoped_lock<std::mutex> lock(m_connectionsMutex);
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
			if (_Type == TypeWorking::Server)
			{
				std::unique_lock<std::mutex> MySQL_Lock(m_MySQL);
				WaitForMySQL.wait(MySQL_Lock);
			}
			auto Lambd = [&](Connection::SharedPtr connection)
			{
				if (!connection)
				{
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
							std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
							m_connectionsTCP.erase(itConnection);
						}
					}
					else if (_Type == TypeWorking::Server && _Proto == TypeProtocol::UDP)
					{
						auto itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
							[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
						{
							if (ThisConn.second == connection)
								return true;
							return false;
						});
						if (itConnection != m_connectionsUDP.end())
						{
							std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
							m_connectionsUDP.erase(itConnection);
						}
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
						if (_Proto == TypeProtocol::TCP)
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
								std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
								User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
									std::to_string(connection->GetMetaDB_User()) + "'" } });
								connection->get_socketTCP().close();
								m_connectionsTCP.erase(itConnection);
							}
						}
						else if (_Proto == TypeProtocol::UDP)
						{
							auto itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
								[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
							{
								if (ThisConn.second == connection)
									return true;
								return false;
							});
							if (itConnection != m_connectionsUDP.end())
							{
								std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
								User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
									std::to_string(connection->GetMetaDB_User()) + "'" } });
								m_connectionsUDP.erase(itConnection);
							}
						}
						return;
					}
				}

				if (connection && (!connection->GetLogged()))
				{
					if (_Type == TypeWorking::Server)
					{
						connection->GetPacket(packet, swl::Packet::Type::MySQL);
						if (packet && (packet.getData().find("_0") != std::string::npos &&
							packet.getData().find("_1") != std::string::npos))
						{
							json temp = json::parse(packet.getData());
							if (!temp.empty())
							{
								std::string Login = temp["_0"].get<std::string>(),
									Pass = temp["_1"].get<std::string>();

								auto Obj = User->TrySelectValues("Local", { "*" },
									{ " WHERE _0 = '" + Login + "' AND _1 = '" + Pass + "'" });

								// If Successful Then Send Answer About It
								swl::Packet Answer = swl::Packet();
								json pack = json::parse(Answer.CreateAnswer()->getData());
								if (!Obj.empty())
								{
									pack["data"]["body"]["_0"] = "OK";

									connection->SetMetaDB_User((int)Obj["_N"].front().get<json::number_integer_t>());

									if (Obj["_2"].front().get<json::number_integer_t>() == 1)
										pack["data"]["body"]["_0"] = "AlreadyOnl";
								}
								else
									pack["data"]["body"]["_0"] = "NotFound";

								Answer.FillIn(swl::Packet::Header(swl::Packet::Type::MySQL), pack);
								connection->Send(Answer);

								if (pack["data"]["body"]["_0"] == "NotFound" ||
									pack["data"]["body"]["_0"] == "AlreadyOnl")
								{
									if (_Proto == TypeProtocol::TCP)
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
											std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
											connection->get_socketTCP().close();
											m_connectionsTCP.erase(itConnection);
										}
									}
									else if (_Proto == TypeProtocol::UDP)
									{
										auto itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
											[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
										{
											if (ThisConn.second == connection)
												return true;
											return false;
										});
										if (itConnection != m_connectionsUDP.end())
										{
											std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
											m_connectionsUDP.erase(itConnection);
										}
									}
									return;
								}
								if (pack["data"]["body"]["_0"] == "OK")
								{
									connection->SetLogged();
									User->TryUpdateValues("Local", { "_2" }, { { "1" } }, { {
										" WHERE _N = '" + std::to_string(connection->GetMetaDB_User()) + "'" } });

									return;
								}
							}
							else
							{
								if (_Proto == TypeProtocol::TCP)
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
										std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
										User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
											std::to_string(connection->GetMetaDB_User()) + "'" } });
										connection->get_socketTCP().close();
										m_connectionsTCP.erase(itConnection);
									}
								}
								else if (_Proto == TypeProtocol::UDP)
								{
									auto itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
										[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
									{
										if (ThisConn.second == connection)
											return true;
										return false;
									});
									if (itConnection != m_connectionsUDP.end())
									{
										std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
										User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
											std::to_string(connection->GetMetaDB_User()) + "'" } });
										m_connectionsUDP.erase(itConnection);
									}
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
								if (Callback_OnLoggin)
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
							connection->SetConnected(true);
						}
					}

					// If Wasn't MySQL Packet And Timer Is Done
					if (_Type == TypeWorking::Server && connection->GetTimer())
					{
						if (_Proto == TypeProtocol::TCP)
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
								User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
									std::to_string(connection->GetMetaDB_User()) + "'" } });
								connection->get_socketTCP().close();
								//BUG -- m_connectionsTCP.erase(itConnection);
							}
						}
						else if (_Proto == TypeProtocol::UDP)
						{
							auto itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
								[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
							{
								if (ThisConn.second == connection)
									return true;
								return false;
							});
							if (itConnection != m_connectionsUDP.end())
							{
								User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
									std::to_string(connection->GetMetaDB_User()) + "'" } });
								//BUG -- m_connectionsUDP.erase(itConnection);
							}
						}
						return;
					}
				}

				if (Callback_OnClientHandler && connection && connection->IsConnected())
					Callback_OnClientHandler(connection);
			};

			while ((!m_io_service.stopped() || IsRunning()) && !isUpdate)
			{
				if (_Type == TypeWorking::Client && one_connection)
				{
					while (one_connection && (one_connection->getIsError() && !one_connection->get_error_queue().empty()))
					{
						Sleep(1000);
						if (Callback_OnError)
						{
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
					std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
					
					Last = std::chrono::high_resolution_clock::now();

					// Check For Are Users Online Now Or Not And Then Try To Disconnect Them
					if (!(_Proto == TypeProtocol::TCP ? !m_connectionsTCP.empty() : !m_connectionsUDP.empty()) && (Last - Curr) > std::chrono::seconds(20))
					{
						Curr = std::chrono::high_resolution_clock::now();
						// Get All Users Who Is Online Now Or Not
						auto IsOnline = User->TrySelectValues("Local", { "_2, _N" });

						if (_Proto == TypeProtocol::TCP)
						{
							std::map<asio::ip::tcp::endpoint, Connection::SharedPtr>::iterator itConnection = m_connectionsTCP.end();
							if (!IsOnline.empty())
							{
								for (auto It : IsOnline)
								{
									// If Isn't Online Then Disconnect It
									if (!It.empty() &&
										(It.find("_N") != It.end() &&
											It.find("_2") != It.end() &&
											It["_2"].get<json::number_integer_t>() == 0))
									{
										itConnection = std::find_if(m_connectionsTCP.begin(), m_connectionsTCP.end(),
											[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
										{
											if (ThisConn.second &&
												(ThisConn.second->GetLogged() || ThisConn.second->IsConnected() &&
													!ThisConn.second->GetStopped()) &&
												ThisConn.second->GetMetaDB_User() == It["_N"].get<json::number_integer_t>())
												return true;
											return false;
										});
									}
								}
							}

							// If Nobody Is Online But Have Connected...
							else
							{
								itConnection = std::find_if(m_connectionsTCP.begin(), m_connectionsTCP.end(),
									[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
								{
									if (ThisConn.second &&
										(ThisConn.second->GetLogged() || ThisConn.second->IsConnected() &&
											!ThisConn.second->GetStopped()))
										return true;
									return false;
								});
							}
							if (itConnection != m_connectionsTCP.end())
							{
								std::scoped_lock<std::mutex> _MainLock(m_connectionsMutex);
								itConnection->second->get_socketTCP().close();
								m_connectionsTCP.erase(itConnection);
							}
						}
						else if (_Proto == TypeProtocol::UDP)
						{
							std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator itConnection = m_connectionsUDP.end();
							if (!IsOnline.empty())
							{
								for (auto It: IsOnline)
								{
									// If Isn't Online Then Disconnect It
									if (!It.empty() &&
										(It.find("_N") != It.end() &&
											It.find("_2") != It.end() &&
											It["_2"].get<json::number_integer_t>() == 0))
									{
										itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
											[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
										{
											if (ThisConn.second &&
												(ThisConn.second->GetLogged() || ThisConn.second->IsConnected() &&
													!ThisConn.second->GetStopped()) &&
												ThisConn.second->GetMetaDB_User() == It["_N"].get<json::number_integer_t>())
												return true;
											return false;
										});
									}
								}
							}

							// If Nobody Is Online But Have Connected...
							else
							{
								itConnection = std::find_if(m_connectionsUDP.begin(), m_connectionsUDP.end(),
									[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
								{
									if (ThisConn.second &&
										(ThisConn.second->GetLogged() || ThisConn.second->IsConnected() &&
											!ThisConn.second->GetStopped()))
										return true;
									return false;
								});
							}
							if (itConnection != m_connectionsUDP.end())
							{
								std::scoped_lock<std::mutex> _MainLock(m_connectionsMutex);
								m_connectionsUDP.erase(itConnection);
							}
						}
					}

					if (_Proto == TypeProtocol::TCP)
					{
						for (auto connection: m_connectionsTCP)
						{
							if (!connection.second) continue;

							if (!connection.second->GetStopped())
								Lambd(connection.second);

							if (connection.second)
							{
								if (connection.second->getIsError() && !connection.second->get_error_queue().empty())
								{
									User->TryUpdateValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
									std::to_string(connection.second->GetMetaDB_User()) + "'" } });
									connection.second->get_error_queue().pop_front();
								}
								connection.second->waiterDisconnection.notify_all();
							}
						}
					}
					else if (_Proto == TypeProtocol::UDP)
					{
						for (auto connection: m_connectionsUDP)
						{
							if (!connection.second) continue;

							if (!connection.second->GetStopped())
								Lambd(connection.second);

							if (connection.second)
							{
								if (connection.second->getIsError() && !connection.second->get_error_queue().empty())
								{
									User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _N = '" +
									std::to_string(connection.second->GetMetaDB_User()) + "'" } });
									connection.second->get_error_queue().pop_front();
								}
								connection.second->waiterDisconnection.notify_all();
							}
						}
					}
				}
			}
		}).detach();
	}
}
