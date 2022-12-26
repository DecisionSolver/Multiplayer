#pragma once

#include "pch.h"

#include <Packet.hpp>
#include <FTPClient.h>
#include <boost/array.hpp>

#include "Cookie.h"

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
	void Send(const void *Data, size_t Size);

	void SetLogged() { isLogged = true; }
	void SetConnected(bool IsConnected) { Connected = IsConnected; }
	
	bool GetLogged() { return isLogged; }
	bool IsConnected() { return Connected; }

	void GetPacket(network::Packet &packet, const int &_CheckingByType,
		const int &_CheckingByStatus = -1,
		const std::string &_CheckingByData = "");

	void GetPacketFromDelayed(network::Packet &packet, const int &_CheckingByType,
		const int &_CheckingByStatus = -1,
		const std::string &_CheckingByData = "");

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

	std::shared_ptr<network::ClientFTP> getFtpClient() { return ftpClient; }

	void SetEndPoint(const asio::ip::udp::endpoint &NewEndPoint) { remote_endpoint_ = NewEndPoint; }
	asio::ip::udp::endpoint remote_endpoint() { return remote_endpoint_; }

	const std::atomic<size_t> &GetCurrentPing() { return ping; }

	ConnectionManager *m_owner = nullptr;

	// It Needs To Hold All Data That Came From Receive (Only There It Needs To Use)
	//asio::streambuf m_receiveData;

	std::atomic_bool HasLeftPackets = false;
	size_t m_clientId = 0;
	
	std::map<int, std::shared_ptr<network::Packet>> packet_queue;

	// When We Need Just Save Some Packets To The Future Use
	std::map<int, std::shared_ptr<network::Packet>> packet_queue_delayed;

	// For Test Purpose
	void DoReceive();
private:
	std::shared_ptr<Cookie> cookie = std::make_shared<Cookie>();

	static size_t m_nextClientId;

#if defined(USE_SSL)
	std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> m_socketTCP;
#else
	std::shared_ptr<asio::ip::tcp::socket> m_socketTCP;
#endif
	std::shared_ptr<asio::ip::udp::socket> m_socketUDP;

	asio::ip::udp::endpoint remote_endpoint_;

	std::atomic<bool> m_stopped;
	enum { max_buffer_lenght = 65536 };
	boost::array<char, max_buffer_lenght> m_receiveData;
	std::vector<char> m_receiveBuffer;

	mutable std::mutex m_disconnect, m_error;

	std::condition_variable error;
	std::atomic_bool IsError = false;

	std::vector<char> m_sendBuffers[2]; // Double buffer
	int m_activeSendBufferIndex = 0;
	bool m_sending = false, isLogged = false, Connected = false;

	std::deque<asio::error_code> error_queue;

	std::vector<char> m_allReadData; // Strictly for test purposes

	std::condition_variable cvReceive, cvSend;
	std::mutex muxReceive, muxSend;
	std::atomic_bool atSend = false, atReceive = false;

	void DisconnectByError(const asio::error_code &errorCode = asio::error_code());
	void DoSend();

	// Only For Server!
	int UserID_MetaDB = 0; // Number Line Of This DB User (Easily Work With User In MySQL)

	std::shared_ptr<network::ClientFTP> ftpClient;

	std::atomic<size_t> ping = 0u;

	// Ping
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::milliseconds ms;
	typedef std::chrono::duration<float> fsec;
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	//

	void ProccessPacket(const std::shared_ptr<Connection> &self);
	void ProccessChain(std::shared_ptr<network::Packet> packet);
};
