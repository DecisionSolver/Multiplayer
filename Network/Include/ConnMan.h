#pragma once
#include "pch.h"
#include "Connection.h"

#include "MySQL/MySQL_Client.h"
#include <fineftp/server.h>
//fineftp::FtpServer server(_IP, 2121, local_root.string(), "Microsoft Access Driver (*.mdb)",
//"F:\\Programming\\C++\\Project\\ODBC\\ODBC\\test.MDB", std::vector<std::string>({ "READONLY=false" }), "12345");

//--------------------------------------------------------------------
class ConnectionManager
{
protected:
	std::string _IP;
	USHORT _Port = 0;

#if defined(USE_SSL)
	bool IsSetupPathsCert_All = false, IsSetupPathsCert_Chain = false, IsSetupPathsCert_Private = false,
		IsSetupPathsCert_DH = false, IsSetupPathsCert_RSA_Private_Key = false;
	const std::string SSL_Cert_Chain, SSL_Private_Key, SSL_TMP_DH, SSL_RSA_Private_Key;
#endif
	void SendPacket(bool ExceptionClient, const Connection::SharedPtr &connection,
		const std::shared_ptr<network::Packet>& Packet);
public:
#if defined(USE_SSL)
	static bool verify_certificate(bool preverified, asio::ssl::verify_context &ctx);
#endif

	enum TypeWorking
	{
		Server = 0,
		Client
	};
	enum class TypeProtocol: int
	{
		TCP = (1 << 0),
		UDP = (1 << 1),
		FTP = (1 << 2),
		VOIP = (1 << 3)
	};

	ConnectionManager(const TypeWorking &_Type, const int Protocol, const std::string &IP, const USHORT &port,
		size_t numThreads = 2);
	ConnectionManager(const ConnectionManager &) = delete;
	ConnectionManager(ConnectionManager &&) = delete;
	ConnectionManager &operator = (const ConnectionManager &) = delete;
	ConnectionManager &operator = (ConnectionManager &&) = delete;
	~ConnectionManager();

	bool StartSystem(const std::function<void(Connection::SharedPtr)> &Func = nullptr);
	void StopSystem();

	void SetIP(const std::string &NewIP) { _IP = NewIP; }
	void SetPort(USHORT NewPort) { _Port = NewPort; }

	// If Server Supports Cookies We Can Send Login And If Server Needs Your Password It Sends Need_To_Login Packet
	bool ConnectToServer(const std::string &Login, const std::string &Pass);
	bool IsRunning() const;
	
	void Send(const std::shared_ptr<network::Packet> &Packet);
	void Send(const std::string &Packet);
	void Send(const void *Data, size_t Size);

	void SetCB_Accept(const std::function<void(Connection::SharedPtr)> &Func);
	void SetCB_OnPacketHandle(const std::function<void(Connection::SharedPtr)> &Func);
	void SetCB_OnLoggin(const std::function<void(Connection::SharedPtr)> &Func);
	void SetCB_OnError(const std::function<void(asio::error_code)> &Func);

	void OnConnectionClosed(Connection::SharedPtr connection, bool Need2DiscFromMySQL = true);
	bool OnConnection(Connection::SharedPtr connection, const std::string &Login,
		const std::string &Pass = std::string());

	std::atomic_bool &isInUpdate() { return isUpdate; };

	// Only CLIENT!
	Connection::SharedPtr GetConnect();
	
	const TypeWorking GetTypeWork() const { return _Type; }
	const int GetProtocol() const { return _Proto; }

	asio::io_service &GetIOService() { return m_io_service; }

	std::condition_variable &IsWait();
	std::condition_variable &IsWaitMySQL() { return WaitForMySQL; }
	
	//asio::ip::udp::socket &GetSocketUDP() { return *m_SocketUDP; }
	static std::map<asio::ip::udp::endpoint, Connection::SharedPtr> m_connectionsUDP;
	static std::map<asio::ip::tcp::endpoint, Connection::SharedPtr> m_connectionsTCP;
	
	std::shared_ptr<mysql::MYSQLCLIENT> MySQL_DB = std::make_shared<mysql::MYSQLCLIENT>();

	// Key
	void Set_Cert_RSA_Private(const std::string &Path)
	{
#if defined(USE_SSL)
		const_cast<std::string &>(SSL_RSA_Private_Key) = Path;
		IsSetupPathsCert_RSA_Private_Key = true;
#else
		UNREFERENCED_PARAMETER(Path);
#endif
	}
	void Set_Cert_Chain(const std::string &Path)
	{
#if defined(USE_SSL)
		const_cast<std::string &>(SSL_Cert_Chain) = Path;
		IsSetupPathsCert_Chain = true;
#else
		UNREFERENCED_PARAMETER(Path);
#endif
	}
	void Set_Private_Key(const std::string &Path)
	{
#if defined(USE_SSL)
		if (_Type == TypeWorking::Client) return;
		const_cast<std::string &>(SSL_Private_Key) = Path;
		IsSetupPathsCert_Private = true;
#else
		UNREFERENCED_PARAMETER(Path);
#endif
	}
	void Set_TMP_DH(const std::string &Path)
	{
#if defined(USE_SSL)
		if (_Type == TypeWorking::Client) return;
		const_cast<std::string &>(SSL_TMP_DH) = Path;
		IsSetupPathsCert_DH = true;
#else
		UNREFERENCED_PARAMETER(Path);
#endif
	}
	void Set_All_Paths(const std::string &Cert_Chain,
		const std::string &Cerf_Private_Key,
		const std::string &TMP_DH,
		const std::string &RSA_Private_Key)
	{
#if defined(USE_SSL)
		if (_Type == TypeWorking::Client) return;
		Set_Cert_RSA_Private(RSA_Private_Key);
		Set_Cert_Chain(Cert_Chain);
		Set_Private_Key(Cerf_Private_Key);
		Set_TMP_DH(TMP_DH);
		IsSetupPathsCert_All = true;
#else
		UNREFERENCED_PARAMETER(Cert_Chain);
		UNREFERENCED_PARAMETER(Cerf_Private_Key);
		UNREFERENCED_PARAMETER(TMP_DH);
		UNREFERENCED_PARAMETER(RSA_Private_Key);
#endif
	}

	struct PoolWaiter
	{
	public:
		enum ReturnType{ Type = 1, Status };
	private:
		std::mutex m_PacketWaiter;
		std::atomic_bool WasPacket = false;
		std::atomic_bool wasActive = false;
		std::atomic_bool NeedToBreak = false;
		std::chrono::time_point<std::chrono::steady_clock> Cur, Last;

		// Packet Type To Check
		//		type, need to break all chain
		std::map<int, bool> TypeToCheck; // ref: network::Packet::Type

		// Packet Status (Only Works With Type)
		//		type, need to break all chain
		std::map<int, bool> StatusToCheck; // ref: network::Packet::Status

		std::pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>> CauseBreak;
	public:
		//std::condition_variable cv_PacketWaiter;

		void SetTypeCauseBreak(const int &Type)
		{
			//							.first		.second			.first			.second
			CauseBreak = std::make_pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>(ReturnType::Type,
				{ (*TypeToCheck.find(Type)), { -1, false } });
		}
		void SetStatusCauseBreak(const int &Type, const int &Status)
		{
			//							.first		.second			.first			.second
			CauseBreak = std::make_pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>(ReturnType::Status,
				{ (*TypeToCheck.find(Type)), (*StatusToCheck.find(Status)) });
		}

		// Set Type To Check In Needed Packet When It Cames
		void SetType(const int &Type, bool need2break = false)
		{
			TypeToCheck.insert({ Type, need2break });
		}
		// Sets When Need To Unblock "Check" Function
		void NotifyOne()
		{
			NeedToBreak.store(true);
		}

		// Sets When Packet Has Come
		void SetWasPacket()
		{
			WasPacket.store(true);
		}
		// Set Status To Check In Needed Packet When It Cames (Only Works With Type!!!)
		void SetStatus(const int &Status, const int &Type, bool need2break = false)
		{
			TypeToCheck.insert({ Type, need2break });
			StatusToCheck.insert({ Status, need2break });
		}

		std::map<int, bool> &GetStatus() { return StatusToCheck; }
		std::map<int, bool> &GetType() { return TypeToCheck; }
		bool WasActive() { return wasActive.load(); }
		//			//was break								// type,				// status
		std::pair<bool, std::pair<ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>>
			Check(const std::chrono::seconds &TimeOut = 60s);
	};

	void EnableNotAllowWithoutCookie();
	void DisableNotAllowWithoutCookie();

	void SetSocketBlocking(bool IsNeed2Block)
	{
		SocketBlocking = IsNeed2Block;
	}
	bool IsSocketBlocking() { return SocketBlocking; }
protected:
	asio::io_service m_io_service;
	std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;
	std::shared_ptr<fineftp::FtpServer> FTPServer;

#if defined(USE_SSL)
	asio::ssl::context Context_SSL;
#endif

	std::vector<std::thread> m_threads;

	mutable std::mutex m_MySQL, m_ConnectedClose;
	Connection::SharedPtr one_connection;

	std::atomic_bool isUpdate = false;
	std::condition_variable waiter_update, WaitForMySQL;

	void IoServiceThreadProc();

	void DoAccept();
	void Handler(std::function<void(Connection::SharedPtr)> Func);

	bool IsWorking = false, NotAllowWithoutCookie = false, SocketBlocking = false;
	TypeWorking _Type;
	int _Proto = (int)TypeProtocol::TCP;

	std::function<void(Connection::SharedPtr)> Callback_OnClientHandler, Callback_Accept, Callback_OnLoggin;
	std::function<void(asio::error_code)> Callback_OnError;

#if defined(USE_SSL)
	std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> newConnTCP_SSL;
#else
	std::shared_ptr<asio::ip::tcp::socket> newConnTCP;
#endif

	std::chrono::time_point<std::chrono::steady_clock> Curr, Last;
	bool GetTimer(const Connection::SharedPtr &User);

	bool LoadConfig();
	
	std::shared_ptr<Cookie> cookie_users = std::make_shared<Cookie>();
};
