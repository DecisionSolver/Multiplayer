#pragma once
#include "pch.h"
#include "Connection.h"

#if defined __has_include && __has_include("asio.hpp")
// Boost Includes
#include "asio.hpp"

// Standard Includes
#include <thread>
#include <vector>
#include <functional>

#include "MySQL/MySQL_Client.h"
#include "MySQL/MySQL_Impl.h"

//--------------------------------------------------------------------
class ConnectionManager
{
protected:
	std::string _IP;
	UINT _Port = 0;
public:
	enum TypeWorking
	{
		Server = 0,
		Client
	};

	ConnectionManager(TypeWorking _Type, std::string IP, UINT port, size_t numThreads = 2);
	ConnectionManager(const ConnectionManager &) = delete;
	ConnectionManager(ConnectionManager &&) = delete;
	ConnectionManager &operator = (const ConnectionManager &) = delete;
	ConnectionManager &operator = (ConnectionManager &&) = delete;
	~ConnectionManager();

	void StartSystem(std::function<void(Connection::SharedPtr)> Func = nullptr);
	void StopSystem();

	void SetIP(std::string NewIP) { _IP = NewIP; }
	void SetPort(UINT NewPort) { _Port = NewPort; }

	void ConnectToServer();
	bool IsRunning() const;

	void Send(std::string Packet);

	void SetCB_Accept(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnPacketHandle(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnLoggin(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnError(std::function<void(asio::error_code)> Func);

	void OnConnectionClosed(Connection::SharedPtr connection);

	// Only SERVER!
	std::vector<Connection::SharedPtr> GetAllConnections();
	
	// Only CLIENT!
	Connection::SharedPtr GetConnect();
	
	const TypeWorking GetTypeWork() const { return _Type; }

	asio::io_service &GetIOService() { return m_io_service; }

	std::condition_variable &IsWait();
protected:
	asio::io_service m_io_service;
	asio::ip::tcp::acceptor m_acceptor;
	std::unique_ptr<asio::ip::tcp::socket> m_Socket;
	std::vector<std::thread> m_threads;

	mutable std::mutex m_connectionsMutex, m_onconn_close, m_get_allconn,
		m_main_handler, m_do_accept, m_stop_sys;
	std::vector<Connection::SharedPtr> m_connections;
	Connection::SharedPtr one_connection;

	void IoServiceThreadProc();

	void DoAccept();
	void Handler(std::function<void(Connection::SharedPtr)> Func);

	bool IsWorking = false;
	TypeWorking _Type;

	std::function<void(Connection::SharedPtr)> Callback_OnClientHandler, Callback_Accept, Callback_OnLoggin;
	std::function<void(asio::error_code)> Callback_OnError;

	std::shared_ptr<mysql::MYSQLCLIENT> User = std::make_shared<mysql::MYSQLCLIENT>();

	std::unique_ptr<asio::ip::tcp::socket> newConn;
};

//--------------------------------------------------------------------
#endif
