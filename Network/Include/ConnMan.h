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
	UINT _Port = 0;
public:
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

	ConnectionManager(TypeWorking _Type, TypeProtocol _Proto, std::string IP, UINT port, size_t numThreads = 2);
	ConnectionManager(const ConnectionManager &) = delete;
	ConnectionManager(ConnectionManager &&) = delete;
	ConnectionManager &operator = (const ConnectionManager &) = delete;
	ConnectionManager &operator = (ConnectionManager &&) = delete;
	~ConnectionManager();

	void StartSystem(std::function<void(Connection::SharedPtr)> Func = nullptr);
	void StopSystem();

	void SetIP(std::string NewIP) { _IP = NewIP; }
	void SetPort(UINT NewPort) { _Port = NewPort; }

	bool ConnectToServer();
	bool IsRunning() const;
	
	void Send(const network::Packet &Packet);
	void Send(const std::string &Packet);

	void SetCB_Accept(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnPacketHandle(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnLoggin(std::function<void(Connection::SharedPtr)> Func);
	void SetCB_OnError(std::function<void(asio::error_code)> Func);

	void OnConnectionClosed(Connection::SharedPtr connection);

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
protected:
	asio::io_service m_io_service;
	asio::ip::tcp::acceptor m_acceptor;
	std::unique_ptr<asio::ip::tcp::socket> m_SocketTCP;

	std::unique_ptr<asio::ip::udp::socket> m_SocketUDP;

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

	std::unique_ptr<asio::ip::tcp::socket> newConnTCP;

	std::chrono::time_point<std::chrono::steady_clock> Curr, Last;
};
