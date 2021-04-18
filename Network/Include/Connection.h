#pragma once
#include "pch.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include <Packet.hpp>
#include <FTPClient.h>

//--------------------------------------------------------------------
class ConnectionManager;

//--------------------------------------------------------------------
class Connection: public std::enable_shared_from_this<Connection>
{
public:
	typedef std::shared_ptr<Connection> SharedPtr;

	// Ensure all instances are created as shared_ptr in order to fulfill requirements for shared_from_this
	static Connection::SharedPtr Create(ConnectionManager *connectionManager, asio::ip::tcp::socket &socket);
	static Connection::SharedPtr Create(ConnectionManager *connectionManager);

	//
	static std::ostringstream ErrorCodeToString(const asio::error_code &errorCode);

	Connection(const Connection &) = delete;
	Connection(Connection &&) = delete;
	Connection &operator = (const Connection &) = delete;
	Connection &operator = (Connection &&) = delete;
	~Connection();

	// We have to defer the start until we are fully constructed because we share_from_this()
	void Start();
	void Stop();

	void Send(const std::vector<char> &data);
	void Send(network::Packet &packet);

	void SetLogged() { isLogged = true; }
	void SetConnected(bool IsConnected) { Connected = IsConnected; }
	
	bool GetLogged() { return isLogged; }

	bool IsConnected() { return Connected; }
	bool GetTimer();

	void GetPacket(network::Packet &packet, network::Packet::Type _CheckingByType, std::string _CheckingByData = "");

	std::mutex &getMutex_Error() { return m_error; }
	std::atomic_bool &getIsError() { return IsError; }
	std::condition_variable &get_cv_error() { return error; }
	std::deque<asio::error_code> &get_error_queue() { return error_queue; }

	std::condition_variable successConn, waiterDisconnection;
	std::atomic<bool> &GetStopped() { return m_stopped; }

	void SetMetaDB_User(int ID) { UserID_MetaDB = ID; }
	int GetMetaDB_User() { return UserID_MetaDB; }

	asio::ip::tcp::socket &get_socketTCP() { return m_socketTCP; }

	std::shared_ptr<FTPClient> getFtpClient() { return ftpClient; }

	void SetEndPoint(asio::ip::udp::endpoint NewEndPoint) { remote_endpoint_ = NewEndPoint; }
	asio::ip::udp::endpoint remote_endpoint() { return remote_endpoint_; }
private:
	static size_t m_nextClientId;
	size_t m_clientId = 0;
	ConnectionManager *m_owner = nullptr;

	asio::ip::tcp::socket m_socketTCP;

	asio::ip::udp::endpoint remote_endpoint_;

	std::atomic<bool> m_stopped;
	asio::streambuf m_receiveBuffer;

	mutable std::mutex m_disconnect, m_error;

	std::condition_variable error;
	std::atomic_bool IsError = false;

	std::vector<char> m_sendBuffers[2]; // Double buffer
	int m_activeSendBufferIndex = 0;
	bool m_sending = false, isLogged = false, Connected = false;

	std::map<network::Packet::Type, network::Packet> packet_queue;
	std::deque<asio::error_code> error_queue;

	std::vector<char> m_allReadData; // Strictly for test purposes

	Connection(ConnectionManager *connectionManager, asio::ip::tcp::socket socket);
	Connection(ConnectionManager *connectionManager);

	void DoReceive();
	void DoSend();
	std::chrono::time_point<std::chrono::steady_clock> Curr, Last;

	int UserID_MetaDB = 0; // Number Line Of This DB User (Easily Work With User In MySQL)

	std::shared_ptr<FTPClient> ftpClient;
};
