#pragma once
#include "pch.h"
#include "Connection.h"

// Boost Includes
#include "asio.hpp"

// Standard Includes
#include <thread>
#include <vector>
#include <functional>

#include "MySQL/MySQL_Client.h"

//--------------------------------------------------------------------
class ConnectionManager
{
protected:
	std::string _IP;
	USHORT _Port = 0;

#if defined(USE_SSL)
	bool IsSetupPathsCert_All = false, IsSetupPathsCert_Chain = false, IsSetupPathsCert_Private = false,
		IsSetupPathsCert_DH = false, IsSetupPathsCert_RSA_Private_Key;
	const std::string SSL_Cert_Chain, SSL_Private_Key, SSL_TMP_DH, SSL_RSA_Private_Key;
#endif
public:
#if defined(USE_SSL)
	static bool verify_certificate(bool preverified, asio::ssl::verify_context &ctx);
#endif
	enum TypeWorking
	{
		Server = 0,
		Client
	};
	enum TypeProtocol
	{
		TCP = 0,
		UDP
	};

	ConnectionManager(TypeWorking _Type, TypeProtocol _Proto, std::string IP, USHORT port, size_t numThreads = 2);
	ConnectionManager(const ConnectionManager &) = delete;
	ConnectionManager(ConnectionManager &&) = delete;
	ConnectionManager &operator = (const ConnectionManager &) = delete;
	ConnectionManager &operator = (ConnectionManager &&) = delete;
	~ConnectionManager();

	void StartSystem(std::function<void(Connection::SharedPtr)> Func = nullptr);
	void StopSystem();

	void SetIP(std::string NewIP) { _IP = NewIP; }
	void SetPort(USHORT NewPort) { _Port = NewPort; }

	bool ConnectToServer();
	bool IsRunning() const;
	
	void Send(const network::Packet &Packet);
	void Send(const std::string &Packet);

	void SetCB_Accept(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnPacketHandle(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnLoggin(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnError(std::function<void(asio::error_code)> Func);

	void OnConnectionClosed(Connection::SharedPtr connection, std::function<void()> Func = nullptr);

	std::atomic_bool &isInUpdate() { return isUpdate; };

	// Only CLIENT!
	Connection::SharedPtr GetConnect();
	
	const TypeWorking GetTypeWork() const { return _Type; }
	const TypeProtocol GetProtocol() const { return _Proto; }

	asio::io_service &GetIOService() { return m_io_service; }

	std::condition_variable &IsWait();
	std::condition_variable &IsWaitMySQL() { return WaitForMySQL; }
	
	asio::ip::udp::socket &GetSocketUDP() { return *m_SocketUDP; }
	static std::map<asio::ip::udp::endpoint, Connection::SharedPtr> m_connectionsUDP;
	static std::map<asio::ip::tcp::endpoint, Connection::SharedPtr> m_connectionsTCP;

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
		const std::string &Private_Key,
		const std::string &TMP_DH)
	{
#if defined(USE_SSL)
		if (_Type == TypeWorking::Client) return;
		const_cast<std::string &>(SSL_Cert_Chain) = Cert_Chain;
		const_cast<std::string &>(SSL_Private_Key) = Private_Key;
		const_cast<std::string &>(SSL_TMP_DH) = TMP_DH;
		IsSetupPathsCert_All = true;
		IsSetupPathsCert_Chain = true;
		IsSetupPathsCert_Private = true;
		IsSetupPathsCert_DH = true;
#else
		UNREFERENCED_PARAMETER(Cert_Chain);
		UNREFERENCED_PARAMETER(Private_Key);
		UNREFERENCED_PARAMETER(TMP_DH);
#endif
	}
protected:
	asio::io_service m_io_service;
	asio::ip::tcp::acceptor m_acceptor;
	
	std::unique_ptr<asio::ip::udp::socket> m_SocketUDP;

#if defined(USE_SSL)
	asio::ssl::context Context_SSL;
#else
	std::unique_ptr<asio::ip::tcp::socket> m_SocketTCP;
#endif

	std::vector<std::thread> m_threads;

	mutable std::mutex m_MySQL;
	Connection::SharedPtr one_connection;

	std::atomic_bool isUpdate = false;
	std::condition_variable waiter_update, WaitForMySQL;

	void IoServiceThreadProc();

	void DoAccept();
	void Handler(std::function<void(Connection::SharedPtr)> Func);

	bool IsWorking = false;
	TypeWorking _Type;
	TypeProtocol _Proto;

	std::function<void(Connection::SharedPtr)> Callback_OnClientHandler, Callback_Accept, Callback_OnLoggin;
	std::function<void(asio::error_code)> Callback_OnError;

	std::shared_ptr<mysql::MYSQLCLIENT> User = std::make_shared<mysql::MYSQLCLIENT>();

#if defined(USE_SSL)
	std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> newConnTCP_SSL;
#else
	std::unique_ptr<asio::ip::tcp::socket> newConnTCP;
#endif

	std::chrono::time_point<std::chrono::steady_clock> Curr, Last;
};
