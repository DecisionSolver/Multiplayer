#pragma once
#include "Socket.hpp"
#include "IPEndpoint.hpp"
#include "Packet.hpp"

namespace swl
{
	class TCPSocket : public Socket
	{
	public:
		TCPSocket();
		TCPSocket(SOCKET& handle);
		~TCPSocket();
		Status listen(const int& backlog = SOMAXCONN);
		Status accept(TCPSocket& socket);
		Status connect(const IPEndpoint& ip, const uint16_t& port);
		Status send(const void* data, const uint32_t& numberBytes, uint32_t& bytesSent);
		Status sendAll(const void* data, const uint32_t& numberBytes);
		Status receive(void* destination, const uint32_t& numberBytes, uint32_t& bytesRecived);
		Status receiveAll(void* destination, const uint32_t& numberBytes);
		Status send(Packet& packet);
		Status receive(Packet& packet);
	};
}