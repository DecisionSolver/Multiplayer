#pragma once
#include "pch.h"
#include "Client.h"

#include "MySQL/MySQL_Client.h"
#include <fineftp/server.h>

class Server
{
protected:
	std::string _IP;
	USHORT Port = 0;

#if defined(USE_SSL)
	bool IsSetupPathsCert_All = false, IsSetupPathsCert_Chain = false, IsSetupPathsCert_Private = false,
		IsSetupPathsCert_DH = false, IsSetupPathsCert_RSA_Private_Key = false;
	const std::string SSL_Cert_Chain, SSL_Private_Key, SSL_TMP_DH, SSL_RSA_Private_Key;
#endif
	void SendPacket(bool ExceptionClient, const std::shared_ptr<Client> &connection,
		const std::shared_ptr<network::Packet>& Packet);
public:
#if defined(USE_SSL)
	static bool verify_certificate(bool preverified, asio::ssl::verify_context &ctx);
#endif

	enum class TypeProtocol: int
	{
		TCP = (1 << 0),
		UDP = (1 << 1),
		FTP = (1 << 2),
	};

	Server(const int Protocol, const std::string &IP, const USHORT &port,
		size_t numThreads = 2);
	Server(const Server &) = delete;
	Server(Server &&) = delete;
	Server &operator = (const Server &) = delete;
	Server &operator = (Server &&) = delete;
	~Server();

	bool StartSystem(const std::function<void(std::shared_ptr<Client>)> &Func = nullptr);
	void StopSystem();

	void SetIP(const std::string &NewIP) { _IP = NewIP; }
	void SetPort(USHORT NewPort) { Port = NewPort; }

	// If Server Supports Cookies We Can Send Login And If Server Needs Your Password It Sends Need_To_Login Packet
	bool ConnectToServer(const std::string &Login, const std::string &Pass);
	bool IsRunning() const;
	
	void Send(const std::shared_ptr<network::Packet> &Packet);
	void Send(const std::string &Packet);
	void Send(const void *Data, size_t Size);

	void Set_OnAccept(const std::function<void(std::shared_ptr<Client>)> &Func);
	void Set_OnPacketHandle(const std::function<void(std::shared_ptr<Client>)> &Func);
	void Set_OnLoggin(const std::function<void(std::shared_ptr<Client>)> &Func);
	void Set_OnError(const std::function<void(asio::error_code)> &Func);

	void OnConnectionClosed(std::shared_ptr<Client> connection, bool Need2DiscFromMySQL = true);
	bool OnConnection(std::shared_ptr<Client> connection, const std::string &Login,
		const std::string &Pass = std::string());

	std::atomic_bool &isInUpdate()
	{
		return isUpdate;
	};

	// Only CLIENT!
	std::shared_ptr<Client> GetConnect();
	
	const int GetProtocol() const
	{
		return _Proto;
	}

	asio::io_service &GetIOService()
	{
		return m_io_service;
	}

	std::condition_variable &IsWait();
	std::condition_variable &IsWaitMySQL()
	{
		return WaitForMySQL;
	}
	
	//asio::ip::udp::socket &GetSocketUDP() { return *m_SocketUDP; }
	static std::map<asio::ip::udp::endpoint, std::shared_ptr<Client>> m_connectionsUDP;
	static std::map<asio::ip::tcp::endpoint, std::shared_ptr<Client>> m_connectionsTCP;
	
	std::shared_ptr<mysql::Client> MySQL_DB = std::make_shared<mysql::Client>();

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
		if (_Type == TypeWorking::Client)
		{
			return;
		}
		const_cast<std::string &>(SSL_Private_Key) = Path;
		IsSetupPathsCert_Private = true;
#else
		UNREFERENCED_PARAMETER(Path);
#endif
	}
	void Set_TMP_DH(const std::string &Path)
	{
#if defined(USE_SSL)
		if (_Type == TypeWorking::Client)
		{
			return;
		}
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
		if (_Type == TypeWorking::Client)
		{
			return;
		}
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

	void EnableNotAllowWithoutCookie();
	void DisableNotAllowWithoutCookie();

	void SetSocketBlocking(bool IsNeed2Block)
	{
		SocketBlocking = IsNeed2Block;
	}
	bool IsSocketBlocking()
	{
		return SocketBlocking;
	}

	std::shared_ptr<fineftp::FtpServer> GetFTPServer()
	{
		return FTPServer;
	}
protected:
	asio::io_service m_io_service;
	std::shared_ptr<asio::ip::tcp::acceptor> m_acceptor;
	std::shared_ptr<fineftp::FtpServer> FTPServer;

#if defined(USE_SSL)
	asio::ssl::context Context_SSL;
#endif

	std::vector<std::thread> m_threads;

	mutable std::mutex m_MySQL, m_ConnectedClose;
	std::shared_ptr<Client> one_connection;

	std::atomic_bool isUpdate = false;
	std::condition_variable waiter_update, WaitForMySQL;

	void IoServiceThreadProc();

	void DoAccept();
	void Handler(std::function<void(std::shared_ptr<Client>)> Func);

	bool IsWorking = false, NotAllowWithoutCookie = false, SocketBlocking = false;
	int _Proto = (int)TypeProtocol::TCP;

	std::function<void(std::shared_ptr<Client>)> Callback_OnClientHandler, Callback_Accept, Callback_OnLoggin;
	std::function<void(asio::error_code)> Callback_OnError;

#if defined(USE_SSL)
	std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> newConnTCP_SSL;
#else
	std::shared_ptr<asio::ip::tcp::socket> newConnTCP;
#endif

	std::chrono::time_point<std::chrono::steady_clock> Curr, Last;
	bool GetTimer(const std::shared_ptr<Client> &User);

	bool LoadConfig();
	
	std::shared_ptr<Cookie> cookie_users = std::make_shared<Cookie>();
};
