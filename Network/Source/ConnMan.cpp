#include "pch.h"
#include "ConnMan.h"
#include <system_error>

#include "Project System/File_system.h"

std::shared_ptr<File_system> FS;

#include "Project System/Project.h"
extern std::unique_ptr<ProjectFile> Project;

std::map<asio::ip::udp::endpoint, Connection::SharedPtr> ConnectionManager::m_connectionsUDP;
std::map<asio::ip::tcp::endpoint, Connection::SharedPtr> ConnectionManager::m_connectionsTCP;

std::mutex m_connectionsMutex;
std::condition_variable cvBlocking, cv_PacketWaiter;
std::mutex muxBlocking, m_PacketWaiter;
std::atomic_bool NoMessageLeft = true, HasConnectionPacket = false;

std::vector<std::shared_ptr<ConnectionManager::PoolWaiter>> PacketChain;

//------------------------------------------------------------------------------
ConnectionManager::ConnectionManager(const TypeWorking &_Type, const int Protocol,
	const std::string &IP, const USHORT &port, size_t numThreads):
	m_io_service()
	, m_threads(numThreads)
	, _IP(IP)
	, _Port(port)
	, _Type(_Type)
#if defined(USE_SSL)
	, Context_SSL(asio::ssl::context::sslv23)
#endif
{
	if (!(Protocol & (int)(TypeProtocol::VOIP)))
	{
		if ((Protocol ^ ((int)(TypeProtocol::VOIP) | (int)(TypeProtocol::TCP)) &&
			Protocol ^ ((int)(TypeProtocol::VOIP) | (int)(TypeProtocol::UDP))) ||
			(Protocol ^ ((int)(TypeProtocol::TCP) | (int)(TypeProtocol::VOIP)) &&
				Protocol ^ ((int)(TypeProtocol::UDP) | (int)(TypeProtocol::VOIP))))
			_Proto = (Protocol | (int)(TypeProtocol::TCP));
		else
			_Proto = (Protocol | (int)(TypeProtocol::UDP));
	}
	else
		_Proto = Protocol;
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

		if (_Proto & (int)(TypeProtocol::FTP))
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
			
			if (_Proto & (int)(TypeProtocol::TCP))
			{
				m_acceptor = std::make_shared<asio::ip::tcp::acceptor>(
					asio::ip::tcp::acceptor(asio::ip::tcp::acceptor(m_io_service,
					asio::ip::tcp::endpoint(asio::ip::address::from_string(_IP), _Port))));
				m_acceptor->set_option(asio::ip::tcp::socket::reuse_address(true));
			}

			std::string Proto = Data.get<std::string>("Server.protocol", "tcp");
			boost::to_lower(Proto);
			if (Proto != "tcp" && Proto != "udp" && Proto.find("voip") == std::string::npos)
				_Proto = (int)(TypeProtocol::TCP);
			else if (Proto.find("voip") != std::string::npos)
				_Proto =(Proto.find("tcp") != std::string::npos
					? (int)(TypeProtocol::TCP) : (int)(TypeProtocol::UDP)) |
				(int)(TypeProtocol::VOIP);
			else
				_Proto = (Proto == "tcp" ? (int)(TypeProtocol::TCP) : (int)(TypeProtocol::UDP));

			cookie_users->SetEnable((bool)Data.get<int>("Server.use_cookie", 1));

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
bool ConnectionManager::StartSystem(const std::function<void(Connection::SharedPtr)> &Func)
{
	if (_Type == TypeWorking::Server)
	{
		if (_Proto & (int)(TypeProtocol::FTP))
		{
			LoadConfig();

			if (FTPServer && FTPServer->start(m_threads.size()))
			{
#if __has_include("logger.h")
				Logger_Info_F("FTP Server Has Been Started On IP {} And Port 2121 And {} Threads", _IP, m_threads.size());
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

			return true;
		}
		else if (!LoadConfig())
		{
#if __has_include("logger.h")
			Logger_Critical("[SERVER] Something Is Wrong With Read Settings From File!");
#endif
			return false;
		}

#if defined(USE_SSL)
		if (!IsSetupPathsCert_All || !(IsSetupPathsCert_Chain && IsSetupPathsCert_Private && IsSetupPathsCert_DH))
		{
#if __has_include("logger.h")
			Logger_Error("Paths to certificates was not set up yet, before 'Start' function you need to call all the following:"\
				"Set_Cert_Chain, Set_Private_Key, Set_TMP_DH\nOr only one: Set_All_Paths!");
#endif
			return false;
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
			return false;

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
	Logger_Warn_F("{} {}\nPort: {}\nProtocol: {}{} Is Secured By SSL? {}",
		_Type == TypeWorking::Server ? "Listening IP:" : "Connected To IP:",
		_IP, _Port,
		_Proto & (int)(TypeProtocol::TCP) ? "TCP\n" : "UDP\n",
		_Proto & (int)(TypeProtocol::VOIP) ? "VOIP On\n" : "", UseSSL);
#endif

	IsWorking = true;

	DoAccept();

	// Log that we are starting the io_service thread
	if (!SocketBlocking)
	{
		for (auto &thread: m_threads)
		{
			if (!thread.joinable())
				thread = std::thread(&ConnectionManager::IoServiceThreadProc, this);
		}
	}

	Handler(Func ? Func : nullptr);

	return true;
}

//------------------------------------------------------------------------------
void ConnectionManager::StopSystem()
{
	IsWorking = false;
	if (_Type == TypeWorking::Server)
	{
#if !defined(USE_SSL)
		if (_Proto & (int)(TypeProtocol::TCP))
			m_connectionsTCP.clear();
#endif
		if (_Proto & (int)(TypeProtocol::UDP))
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

		for (size_t i = 0; i < PacketChain.size(); i++)
		{
			PacketChain[i]->NotifyOne();
		}
		PacketChain.clear();
	}
	if (_Type == TypeWorking::Server)
	{
		// Clear All The Cookies
		MySQL_DB->UpdateValues("", std::vector<std::string>{ { "_6" } }, {{"NULL"}});
	}

	NoMessageLeft = false;
	cvBlocking.notify_one();

	m_io_service.stop();

	for (auto &thread: m_threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

#include "http/HTTP.h"
// Client Only
bool ConnectionManager::ConnectToServer(const std::string &Login, const std::string &Pass)
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
	AnswerPacket->CreatePacket((int)network::Packet::Type::Connection, true,
		{ { "_1", network::Packet::Status::OK }, { "_0", md5_from_buffer(Login) } });

	// Get Info About Client's IP And Send It To Server
	/*{
		HTTP DetailOfClient = HTTP();
		nlohmann::json dt = nlohmann::json::parse(
			DetailOfClient.GET("http://ip-api.com/json")),
			toSend;

		if (dt.find("country_code") != dt.end())
			toSend += { dt["country_code"] };
		if (dt.find("ip") != dt.end())
			toSend += { dt["ip"] };

		if (dt.find("countryCode") != dt.end())
			toSend += { dt["countryCode"] };
		if (dt.find("query") != dt.end())
			toSend += { dt["query"] };

		AnswerPacket->getData()["header"]["cookie"] = StringHEX(SHA256(toSend.dump()));
	}*/

	asio::ip::tcp::resolver resolver(m_io_service);
	auto resolver_data = resolver.resolve(_IP, std::to_string(_Port));

	// Create the connection from a connected socket
	if (_Proto & (int)(TypeProtocol::TCP))
	{
#if defined(USE_SSL)
		newConnTCP_SSL.reset(new asio::ssl::stream<asio::ip::tcp::socket>(m_io_service, Context_SSL));
#else
		one_connection = std::make_shared<Connection>(this, m_io_service);
		asio::connect(*one_connection->get_socketTCP(), resolver_data);
#endif

#if defined(USE_SSL)
		one_connection->get_socketTCP().set_verify_mode(asio::ssl::verify_peer);
		one_connection->get_socketTCP().set_verify_callback(ConnectionManager::verify_certificate);

		one_connection->get_socketTCP().lowest_layer().connect(tcp::endpoint(asio::ip::address::from_string(_IP), _Port), ec);
#endif
	}
	else if (_Proto & (int)(TypeProtocol::UDP))
	{
		auto EndPoint = asio::ip::udp::endpoint(asio::ip::address::from_string(_IP), _Port);
		one_connection = std::make_shared<Connection>(this, m_io_service, EndPoint);
		one_connection->get_socketUDP()->open(asio::ip::udp::v4(), ec);
		one_connection->SetEndPoint(EndPoint);

		//if (!(_Proto & (int)(TypeProtocol::VOIP)))
		one_connection->get_socketUDP()->send_to(asio::buffer(AnswerPacket->getData().dump()), EndPoint);
		if (one_connection->m_owner && one_connection->m_owner->IsSocketBlocking())
			one_connection->DoReceive();
	}

	StartSystem();

	std::this_thread::sleep_for(100ms);

#if defined(USE_SSL)
	if (!ec || (one_connection && one_connection->get_socketTCP().lowest_layer().is_open())
#else
	if (!ec && (_Proto & (int)(TypeProtocol::TCP) && one_connection->get_socketTCP()->is_open())

#endif
	 || (_Proto & (int)(TypeProtocol::UDP) && one_connection->get_socketUDP()->is_open()))
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
		if (_Proto ^ (int)(TypeProtocol::VOIP))
		{
#if __has_include("logger.h")
			Logger_Info("[CLIENT] Sending Back-Response And Waif For Connection Answer");
#endif
		}

		// This is the chain of packets
		// We're here wait for Connection Packet From Server (when it's OK)
		// Then wait for NeedToLogin Packet and after that we can use the system!
		PacketChain.clear();

		int type = (int)network::Packet::Type::Connection >> (int)network::Packet::Type::Login;

		PacketChain.push_back(std::make_shared<PoolWaiter>());
		PacketChain.back()->SetType(type);
		PacketChain.back()->SetStatus((int)network::Packet::Status::OK, type);
		PacketChain.back()->SetStatus((int)network::Packet::Status::Need_To_LogIn << (int)network::Packet::Status::OK, type);

		if (_Proto & (int)(TypeProtocol::TCP)/* && !(_Proto & (int)(TypeProtocol::VOIP))*/)
		{
			one_connection->Send(AnswerPacket);
			if (one_connection->m_owner && one_connection->m_owner->IsSocketBlocking())
				one_connection->DoReceive();
		}

		auto Check = PacketChain.at(0)->Check();
		if (Check.first) // .at() checks the size of a massive (BE_care_FUL!!)
		{
			PacketChain.push_back(std::make_shared<PoolWaiter>());
			PacketChain.back()->SetStatus((int)network::Packet::Status::Need_To_LogIn << (int)network::Packet::Status::OK,
				(int)network::Packet::Type::Login);

			// If Will This Packet Would Receive Then All Chain Will Break And Check Returns true
			PacketChain.back()->SetStatus((int)network::Packet::Status::OK, (int)network::Packet::Type::Login, true);

			if (Check.second.first == ConnectionManager::PoolWaiter::ReturnType::Status &&
				Check.second.second.first.first == (int)network::Packet::Status::OK &&
				Check.second.second.second.first == type)
				return true;

			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket((int)network::Packet::Type::Login, true,
				{ { "_0", network::Packet::Status::Need_To_LogIn }, { "_1", Crypt(Pass) },
				{ "_2", md5_from_buffer(Login) } });

			// To the future if server will need in Need_To_Login Packet!
			one_connection->packet_queue_delayed.insert({ (int)network::Packet::Type::Login, Answer });

			Check = PacketChain.at(1)->Check();
			if (Check.first) // .at() checks the size of a massive (BE_care_FUL!!)
			{
				// We No Need It After Successful Connection And Login!
				one_connection->packet_queue_delayed.erase(
					one_connection->packet_queue_delayed.find((int)network::Packet::Type::Login));
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
	{
#if __has_include("logger.h")
		Logger_Error_F("[CLIENT] Failed Connecting! Error: {}\nAbort Connecting!", Connection::ErrorCodeToString(ec).str());
#endif

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

void ConnectionManager::Send(const std::string &Packet)
{
	if (_Type == ConnectionManager::TypeWorking::Client)
	{
		if (one_connection)
		{
			one_connection->Send({ Packet.begin(), Packet.end() });
			if (one_connection->m_owner && one_connection->m_owner->IsSocketBlocking())
				one_connection->DoReceive();
		}
	}
	else
	{
		if (_Proto & (int)(TypeProtocol::TCP))
		{
#if defined(USE_SSL)
			for (auto &connection: m_connectionsTCP)
#else
			for (auto connection: m_connectionsTCP)
#endif
			{
				connection.second->Send({ Packet.begin(), Packet.end() });
				if (connection.second->m_owner && connection.second->m_owner->IsSocketBlocking())
					connection.second->DoReceive();
			}
		}
		if (_Proto & (int)(TypeProtocol::UDP))
		{
			for (auto connection: m_connectionsUDP)
			{
				connection.second->Send({ Packet.begin(), Packet.end() });
				if (connection.second->m_owner && connection.second->m_owner->IsSocketBlocking())
					connection.second->DoReceive();
			}
		}
	}
}
void ConnectionManager::Send(const void *Data, size_t Size)
{
	if (_Type == ConnectionManager::TypeWorking::Client)
	{
		if (one_connection)
		{
			one_connection->Send(Data, Size);
			if (one_connection->m_owner && one_connection->m_owner->IsSocketBlocking())
				one_connection->DoReceive();
		}
	}
	else
	{
		if (_Proto & (int)(TypeProtocol::TCP))
		{
#if defined(USE_SSL)
			for (auto &connection: m_connectionsTCP)
#else
			for (auto connection: m_connectionsTCP)
#endif
			{
				connection.second->Send(Data, Size);
				if (connection.second->m_owner && connection.second->m_owner->IsSocketBlocking())
					connection.second->DoReceive();
			}
		}
		if (_Proto & (int)(TypeProtocol::UDP))
		{
			for (auto connection: m_connectionsUDP)
			{
				connection.second->Send(Data, Size);
				if (connection.second->m_owner && connection.second->m_owner->IsSocketBlocking())
					connection.second->DoReceive();
			}
		}
	}
}
void ConnectionManager::Send(const std::shared_ptr<network::Packet> &Packet)
{
	if (_Type == ConnectionManager::TypeWorking::Client)
	{
		if (one_connection)
		{
			one_connection->Send(Packet);
			if (one_connection->m_owner && one_connection->m_owner->IsSocketBlocking())
				one_connection->DoReceive();
		}
	}
	else
	{
		if (_Proto & (int)(TypeProtocol::TCP))
		{
#if defined(USE_SSL)
			for (auto &connection: m_connectionsTCP)
#else
			for (auto connection: m_connectionsTCP)
#endif
			{
				connection.second->Send(Packet);
				if (connection.second->m_owner && connection.second->m_owner->IsSocketBlocking())
					connection.second->DoReceive();
			}
		}
		if (_Proto & (int)(TypeProtocol::UDP))
		{
			for (auto connection: m_connectionsUDP)
			{
				connection.second->Send(Packet);
				if (connection.second->m_owner && connection.second->m_owner->IsSocketBlocking())
					connection.second->DoReceive();
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
// It seems just like a one user but it's a server worker xD
Connection::SharedPtr OneConnForServerUDP;

//------------------------------------------------------------------------------
void ConnectionManager::DoAccept()
{
	if (_Type == TypeWorking::Client) return;

	if (_Proto & (int)(TypeProtocol::TCP))
	{
		auto Lambd = 
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

			// Correct The Time
			Curr = Last = std::chrono::high_resolution_clock::now() - 10s;

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
				if (!SocketBlocking)
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

			if (!SocketBlocking)
				DoAccept();
		};

		if (!SocketBlocking)
		{
#if defined(USE_SSL)
			newConnTCP_SSL.reset(new asio::ssl::stream<asio::ip::tcp::socket>(m_io_service, Context_SSL));
#else
			newConnTCP.reset(new asio::ip::tcp::socket(m_io_service));
#endif

#if defined(USE_SSL)
			m_acceptor->async_accept(newConnTCP_SSL->lowest_layer(),
#else
			m_acceptor->async_accept(*newConnTCP, Lambd);
#endif
		}
		else
		{
			std::thread([self = this, Lambd]
			{
				while (self->IsRunning())
				{
					asio::error_code ec;
					if (!self->newConnTCP)
						self->newConnTCP = std::make_shared<asio::ip::tcp::socket>(self->m_acceptor->accept(ec));
					else
						*self->newConnTCP = self->m_acceptor->accept(ec);
					Lambd(ec);
				}
			}).detach();
		}
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

	std::this_thread::sleep_for(100ms);

	if (_Type == TypeWorking::Client)
	{
#if __has_include("logger.h")
		Logger_Info("[CLIENT] Disconnect Client!");
#endif
		connection->successConn.notify_all();

		if (connection->getIsError() &&
			!connection->get_error_queue().empty())
			connection->get_error_queue().pop_front();

		for (size_t i = 0; i < PacketChain.size(); i++)
		{
			PacketChain[i]->NotifyOne();
		}
		PacketChain.clear();

		if (connection)
			connection.reset();
		return;
	}

#if __has_include("logger.h")
	Logger_Info("[SERVER] Disconnect Client!");
#endif
	if (_Type == TypeWorking::Server && _Proto & (int)(TypeProtocol::TCP))
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
				MySQL_DB->UpdateValues("", std::vector<std::string>{ "_2" }, {{"0"}}, {{" WHERE _N = '" +
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

bool ConnectionManager::OnConnection(Connection::SharedPtr connection, const std::string &Login,
	const std::string &Pass)
{
	if (_Type == ConnectionManager::TypeWorking::Client) return false;

	// If Successful Then Send Answer About It
	std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();

	if (Login.empty()) return false;

	bool NeedToUpdateCookie = true;

	if (cookie_users->IsEnable() && NotAllowWithoutCookie)
	{
		// Check If Cookie Was Already Added\Updated In DB
		auto Obj = MySQL_DB->SelectValues("", std::vector<std::string>{ "_6" },
			{ " WHERE _0 = '" + Login + "'" });

		if ((Obj.empty() // There's NO New Cookie
			&&
			// Then Check It Locally
			!cookie_users->FindCookie(connection->get_socketTCP()->remote_endpoint().address().to_string()))
			||
			// Or If New Cookies
			!cookie_users->FindCookie(Obj["_6"].front().get<nlohmann::json::string_t>()))
		{
			// If NOT All Cookies Current User
			Answer->CreatePacket((int)network::Packet::Type::Login, true,
				{ { "_0", network::Packet::Status::NotAllowWithoutCookie } });
			connection->Send(Answer);
			if (connection->m_owner && connection->m_owner->IsSocketBlocking())
				connection->DoReceive();
			OnConnectionClosed(connection, false);
			return false;
		}
		// If Cookie Has Only Locally (Needs To Pull It To DB Latelly)
		else if (Obj.empty() || !cookie_users->FindCookie(Obj["_6"].front().get<nlohmann::json::string_t>()))
			NeedToUpdateCookie = false;
	}
	
	nlohmann::json Obj;
	if (!Pass.empty())
		Obj = MySQL_DB->SelectValues("", std::vector<std::string>{ "*" },
		{ " WHERE _0 = '" + Login + "' AND _1 = '" + Pass + "'" });
	else
		Obj = MySQL_DB->SelectValues("", std::vector<std::string>{ "*" },
		{ " WHERE _0 = '" + Login + "'" });

	nlohmann::json pack = Answer->CreatePacket((int)network::Packet::Type::Login)->getData();
	if (!Obj.empty())
	{
		pack["_0"] = network::Packet::Status::OK;

		connection->SetMetaDB_User((int)Obj["_N"].front().get<nlohmann::json::number_integer_t>());

		if (Obj["_2"].front().get<nlohmann::json::number_integer_t>() == 1)
			pack["_0"] = network::Packet::Status::Already_Online;
	}
	else
		pack["_0"] = network::Packet::Status::Not_Found;
	Answer->FillIn(pack);

	connection->Send(Answer);
	if (connection->m_owner && connection->m_owner->IsSocketBlocking())
		connection->DoReceive();

	if (pack["_0"] == network::Packet::Status::Not_Found ||
		pack["_0"] == network::Packet::Status::Already_Online)
	{
		OnConnectionClosed(connection, false);
		return false;
	}
	if (pack["_0"] == network::Packet::Status::OK)
	{
		connection->SetLogged();

		if (NeedToUpdateCookie)
		{
			auto NewCookie = std::make_shared<Cookie::Struct>();
			NewCookie->Name = "Login";
			NewCookie->Domain = "*"; // That Means All Servers
			NewCookie->Value = String2HEX("Login * " + Login);

			std::string BuildedCookie =
				cookie_users->AddCookie(connection->get_socketTCP()->remote_endpoint().address().to_string(),
					NewCookie);

			// Set That User Is Online And Set Cookie
			MySQL_DB->UpdateValues("", std::vector<std::string>{ "_2", "_6" }, { { "1", BuildedCookie } }, { {
				" WHERE _N = '" +
				std::to_string(connection->GetMetaDB_User()) + "'" } });
		}

		return true;
	}
	return false;
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

	// Hardcoded TIME!
	if (Diff > (1min + 20s))
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
					connection->GetPacket(packet, (int)network::Packet::Type::Disconnection);
					if (packet)
					{
						packet.clear();
						
						OnConnectionClosed(connection);
						return false;
					}
				}

				connection->GetPacket(packet, (int)network::Packet::Type::Connection);
				if (!packet)
					connection->GetPacket(packet, (int)network::Packet::Type::Connection >> (int)network::Packet::Type::Login);
				if (packet)
				{
					if (_Type == TypeWorking::Client && !connection->IsConnected())
					{
						Logger_Warn_F("[CLIENT] We're get the network::Packet::Type::Connection and try to check 'OK',"\
							" from this data: {}", packet.getData().dump());
						if (!packet.getData().empty() &&
							packet.getData()["_0"].get<nlohmann::json::number_integer_t>()
							== (int)network::Packet::Status::OK)
						{
							connection->SetConnected(true);
							connection->successConn.notify_all();
						}
					}
					else
					{
						/* Sending ACCEPT CONNECTION Packet */
						std::shared_ptr<network::Packet> AnswerPacket = std::make_shared<network::Packet>();
						int TypeOfStatus = (int)network::Packet::Status::OK;
						nlohmann::json Data = packet.getData();
						if (Data.find("_0") != Data.end()) // If Has Login To Check Cookie
						{
							std::string Login = Data["_0"].get<std::string>();
							// Check If We Have Cookie This Client Or Need To Make It
							if (!Login.empty() &&
								!cookie_users->FindCookie(connection->get_socketTCP()->remote_endpoint().address().to_string()))
							{
								if (cookie_users->IsEnable() && NotAllowWithoutCookie)
								{
									// Get User's Cookie
									auto Obj = MySQL_DB->SelectValues("", std::vector<std::string>{ "_6" },
										{ " WHERE _0 = '" + Login + "'" });

									// Check And Add New Cookie
									if (!Obj.empty())
									{
										if (!cookie_users->FindCookie(Obj["_6"].front().get<nlohmann::json::string_t>()))
										{
											cookie_users->AddCookie(connection->get_socketTCP()->remote_endpoint().address().to_string(),
												std::make_shared<Cookie::Struct>(
													Cookie::ReadCookie(Obj["_6"].front().get<nlohmann::json::string_t>())));
										}
									}
									else
									{
										AnswerPacket->CreatePacket((int)network::Packet::Type::Login, true,
											{ { "_1", (int)network::Packet::Status::NotAllowWithoutCookie } });
										connection->Send(AnswerPacket);
										if (connection->m_owner && connection->m_owner->IsSocketBlocking())
											connection->DoReceive();
										OnConnectionClosed(connection, false);
										return false;
									}
								}
								else
									TypeOfStatus = (int)network::Packet::Status::Need_To_LogIn << (int)network::Packet::Status::OK;
							}
						}

						connection->SetConnected(true);

						AnswerPacket->CreatePacket((int)network::Packet::Type::Connection >> (int)network::Packet::Type::Login, true,
							{ { "_0", (int)network::Packet::Status::OK }, { "_1", TypeOfStatus } });
						connection->Send(AnswerPacket);
						if (connection->m_owner && connection->m_owner->IsSocketBlocking())
							connection->DoReceive();

						if (TypeOfStatus == (int)network::Packet::Status::OK)
						{
							connection->SetLogged();
							if (!OnConnection(connection, Data["_0"].get<nlohmann::json::string_t>()))
								return false;
							else
								return true;
						}

						packet.clear();
					}
				}

				if (_Type == TypeWorking::Client && !connection->GetLogged())
				{
					// Clear The Login Packet That Signal About Good Or Fail Log in (Came From Server)
					if (packet)
					{
						connection->SetLogged();
						
						if (Callback_OnLoggin)
							Callback_OnLoggin(connection);

						packet.clear();
					}
					connection->GetPacket(packet, (int)network::Packet::Type::Login,
						(int)network::Packet::Status::Need_To_LogIn << (int)network::Packet::Status::OK);
					if (packet && packet.getHeader().IsAnswer)
					{
						// We Need To Send In This Packet Client's Password
						
						network::Packet packetToSend = network::Packet();
						connection->GetPacketFromDelayed(packetToSend, (int)network::Packet::Type::Login,
							(int)network::Packet::Status::Need_To_LogIn);

						std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
						Answer->operator=(packetToSend);
						
						connection->Send(Answer);
						if (connection->m_owner && connection->m_owner->IsSocketBlocking())
							connection->DoReceive();

						packet.clear();
					}
					connection->GetPacket(packet, (int)network::Packet::Type::Login,
						(int)network::Packet::Status::NotAllowWithoutCookie);
					if (packet)
					{
						//network::Packet::PacketReasons.find(network::Packet::Status::NotAllowWithoutCookie)
						//->second
						return false;
					}
					packet.clear();
				}
				if (_Type == TypeWorking::Server && !connection->GetLogged())
				{
					connection->GetPacket(packet, (int)network::Packet::Type::Login, (int)network::Packet::Status::Need_To_LogIn);
					if (packet && packet.getHeader().IsAnswer)
					{
						if (!OnConnection(connection, packet.getData()["_2"].get<nlohmann::json::string_t>(),
							packet.getData()["_1"].get<nlohmann::json::string_t>()))
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
				std::unique_lock<std::mutex> MySQL_Lock(m_MySQL);
				WaitForMySQL.wait(MySQL_Lock);
				std::thread([&]
				{
					while ((!m_io_service.stopped() || IsRunning()) && !isUpdate)
					{
						std::this_thread::sleep_for(500ms);

						if (_Proto & (int)(TypeProtocol::TCP) && m_connectionsTCP.empty() ||
							_Proto & (int)(TypeProtocol::UDP) && m_connectionsUDP.empty()) continue;

						if (!IsSocketBlocking())
							std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
						if (_Proto & (int)(TypeProtocol::TCP))
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
											Logger_Info("[SERVER] User Has Disconnected By Left Time Waiting For Login Packet!");

											std::shared_ptr<network::Packet> AnswerPacket = std::make_shared<network::Packet>();
											AnswerPacket->CreatePacket((int)network::Packet::Type::Disconnection,
												true,
												{ { "_0", network::Packet::Status::TimeOut_LogIn /* Move it to CLIENT! "[SERVER] " +
												network::Packet::PacketReasons.find(network::Packet::Status::TimeOut_LogIn)
												->second*/ } });

											connection->second->Send(AnswerPacket);
											if (connection->second->m_owner && connection->second->m_owner->IsSocketBlocking())
												connection->second->DoReceive();
											OnConnectionClosed(connection->second);

											connection = m_connectionsTCP.begin();

											continue;
										}
										// Check If We Connected In MySQL
										else if (connection->second->GetLogged())
										{
											auto Obj = MySQL_DB->SelectValues("", std::vector<std::string>{ "_2" }, { { " WHERE _N = '" +
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
									connection++;
								}
							}
							if (_Proto & (int)(TypeProtocol::UDP))
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
												Logger_Info("[SERVER] User Has Disconnected By Left Time Waiting For Login Packet!");
												OnConnectionClosed(connection->second);
												connection = m_connectionsUDP.begin();

												continue;
											}
										}
										// Check If We Connected In MySQL
										else if (connection->second->GetLogged())
										{
											auto Obj = MySQL_DB->SelectValues("", std::vector<std::string>{ "_2" }, { {" WHERE _N = '" +
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
									connection++;
								}
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
						std::this_thread::sleep_for(1s);
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
					if (_Proto & (int)(TypeProtocol::TCP))
					{
						{
							//if (!IsSocketBlocking())
							std::scoped_lock<std::mutex> MainLock(m_connectionsMutex);
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
					if (_Proto & (int)(TypeProtocol::UDP))
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
				if (!SocketBlocking)
				{
					while ((_Type == TypeWorking::Server &&
						(_Proto & (int)(TypeProtocol::TCP) ?
							m_connectionsTCP.empty() :
							m_connectionsUDP.empty())
						)
						|| NoMessageLeft)
					{
						std::unique_lock<std::mutex> ul(muxBlocking);
						cvBlocking.wait(ul);
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
	
#if __has_include("logger.h")
	Logger_Info_F("Verifying {}", subject_name);
#endif

	return preverified;
}
#endif

void ConnectionManager::SendPacket(bool ExceptionClient, const Connection::SharedPtr &connection,
	const std::shared_ptr<network::Packet> &Packet)
{
#if !defined(USE_SSL)
	if (_Proto & (int)(TypeProtocol::TCP))
	{
		for (const auto &Next: m_connectionsTCP)
		{
			// Not To ME!!!
			if (ExceptionClient && (connection && connection == Next.second)) continue;
			Next.second->Send(Packet);
			if (connection->m_owner && connection->m_owner->IsSocketBlocking())
				connection->DoReceive();
			if (Next.second->m_owner && Next.second->m_owner->IsSocketBlocking())
				Next.second->DoReceive();
		}
	}
#endif 
	if (_Proto & (int)(TypeProtocol::UDP))
	{
		for (const auto &Next: m_connectionsUDP)
		{
			// Not To ME!!!
			if (ExceptionClient && (connection && connection == Next.second)) continue;
			Next.second->Send(Packet);
			if (Next.second->m_owner && Next.second->m_owner->IsSocketBlocking())
				Next.second->DoReceive();
		}
	}
}

std::pair<bool, std::pair<ConnectionManager::PoolWaiter::ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>>
ConnectionManager::PoolWaiter::Check(const std::chrono::seconds &TimeOut)
{
	std::unique_lock<std::mutex> ul(m_PacketWaiter);
	
	Cur = Last = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds Diff = (Last - Cur);

	// Create Loop For Non-block The Main Thread (to receive packets)
	while (Diff <= TimeOut)
	{
		Last = std::chrono::high_resolution_clock::now();
		Diff = (Last - Cur);

		// Wait For Acception Connection Packet From Server
		std::this_thread::sleep_for(100ms);

		if (NeedToBreak.load())
			break;
	}
	// If It Wasn't Triggered By Time-Out
	if (WasPacket.load() && !wasActive.load())
	{
		wasActive.store(true);
		return { true, CauseBreak };
	}
	else
		return { false, CauseBreak };
	return { false, CauseBreak };
}

void ConnectionManager::EnableNotAllowWithoutCookie()
{
	// Sets When Only One Server Is Allowed To Connect Without Cookie To Set Cookies For Other Servers!
	// Define Some New Servers When Will Need Set This Condition!
	if (_Type != TypeWorking::Server &&
		(!(_Proto & (int)(TypeProtocol::VOIP)) && !cookie_users->IsEnable()))
		return;

	NotAllowWithoutCookie = true;
}

void ConnectionManager::DisableNotAllowWithoutCookie()
{	// Sets When Only One Server Is Allowed To Connect Without Cookie To Set Cookies For Other Servers!
	// Define Some New Servers When Will Need Set This Condition!
	if (_Type != TypeWorking::Server &&
		(!(_Proto & (int)(TypeProtocol::VOIP)) && !cookie_users->IsEnable()))
		return;

	NotAllowWithoutCookie = false;
}
