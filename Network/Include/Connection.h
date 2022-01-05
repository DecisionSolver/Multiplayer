#pragma once

#include "pch.h"

#include <Packet.hpp>
#include <FTPClient.h>
#include <boost/array.hpp>

#if defined(USE_SSL)
#include <asio/ssl.hpp>
#endif

#include <asio.hpp>

//--------------------------------------------------------------------
class ConnectionManager;

//--------------------------------------------------------------------
class Connection: public std::enable_shared_from_this<Connection>
{
public:
	typedef std::shared_ptr<Connection> SharedPtr;

#if defined(USE_SSL)
	Connection(ConnectionManager *connectionManager, asio::io_service &IO,
		std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket);
#else
	Connection(ConnectionManager *connectionManager, asio::io_service &IO,
		std::shared_ptr<asio::ip::tcp::socket> socket);

	Connection(ConnectionManager *connectionManager, asio::io_service &IO);
	Connection(ConnectionManager *connectionManager, asio::io_service &IO, const asio::ip::udp::endpoint &ep);
#endif

	//
	static std::ostringstream ErrorCodeToString(const asio::error_code &errorCode);

	Connection(const Connection &) = delete;
	Connection(Connection &&) = delete;
	Connection &operator = (const Connection &) = delete;
	Connection &operator = (Connection &&) = delete;
	~Connection();

	// We have to defer the start until we are fully constructed because we share_from_this()
	void Start();
	void Stop(bool NeedLock = true);

	void Send(const std::vector<char> &data);
	void Send(std::shared_ptr<network::Packet> packet);

	void SetLogged() { isLogged = true; }
	void SetConnected(bool IsConnected) { Connected = IsConnected; }
	
	bool GetLogged() { return isLogged; }
	bool IsConnected() { return Connected; }

	void GetPacket(network::Packet &packet, network::Packet::Type _CheckingByType, const std::string &_CheckingByData = "");

	std::mutex &getMutex_Error() { return m_error; }
	std::atomic_bool &getIsError() { return IsError; }
	std::condition_variable &get_cv_error() { return error; }
	std::deque<asio::error_code> &get_error_queue() { return error_queue; }

	std::condition_variable successConn/*, waiterDisconnection*/;
	std::atomic<bool> &GetStopped() { return m_stopped; }

	void SetMetaDB_User(int ID) { UserID_MetaDB = ID; }
	int GetMetaDB_User() { return UserID_MetaDB; }

#if defined(USE_SSL)
	asio::ssl::stream<asio::ip::tcp::socket> &get_socketTCP() { return *m_socketTCP; }
#else
	const std::shared_ptr<asio::ip::tcp::socket> &get_socketTCP() { return m_socketTCP; }
	const std::shared_ptr<asio::ip::udp::socket> &get_socketUDP() { return m_socketUDP; }
	void setSocketUDP(const std::shared_ptr<asio::ip::udp::socket> &sock)
	{
		if (m_socketUDP)
			m_socketUDP.reset();
		m_socketUDP = sock;
	}
#endif

	std::shared_ptr<FTPClient> getFtpClient() { return ftpClient; }

	void SetEndPoint(asio::ip::udp::endpoint NewEndPoint) { remote_endpoint_ = NewEndPoint; }
	asio::ip::udp::endpoint remote_endpoint() { return remote_endpoint_; }

	const std::atomic<size_t> &GetCurrentPing() { return ping; }

	ConnectionManager *m_owner = nullptr;

	// It Needs To Hold All Data That Came From Receive (Only There It Needs To Use)
	std::stringstream m_receiveData;

	std::atomic_bool HasLeftPackets = false;
	size_t m_clientId = 0;
	
	std::map<network::Packet::Type, std::shared_ptr<network::Packet>> packet_queue;
private:
	static size_t m_nextClientId;

#if defined(USE_SSL)
	std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> m_socketTCP;
#else
	std::shared_ptr<asio::ip::tcp::socket> m_socketTCP;
#endif
	std::shared_ptr<asio::ip::udp::socket> m_socketUDP;

	asio::ip::udp::endpoint remote_endpoint_;

	std::atomic<bool> m_stopped;
	boost::array<char, 1024> m_receiveBuffer;

	mutable std::mutex m_disconnect, m_error;

	std::condition_variable error;
	std::atomic_bool IsError = false;

	std::vector<char> m_sendBuffers[2]; // Double buffer
	int m_activeSendBufferIndex = 0;
	bool m_sending = false, isLogged = false, Connected = false;

	std::deque<asio::error_code> error_queue;

	std::vector<char> m_allReadData; // Strictly for test purposes

	void DoReceive();
	void DoSend();

	int UserID_MetaDB = 0; // Number Line Of This DB User (Easily Work With User In MySQL)

	std::shared_ptr<FTPClient> ftpClient;

	std::atomic<size_t> ping = 0u;

	// Ping
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::milliseconds ms;
	typedef std::chrono::duration<float> fsec;
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	//

	void ProccessPacket(const std::shared_ptr<Connection> &self);
};
