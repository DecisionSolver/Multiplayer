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
		~TCPSocket() {}
		Status listen(const int& backlog = SOMAXCONN);
		Status accept(TCPSocket& socket);
		Status connect(const IPEndpoint& ip, const uint16_t& port);
		Status SendTo(SOCKET Where, std::shared_ptr<Packet> packet);
		Status send(const char* data, const uint32_t& numberBytes, uint32_t& bytesSent,
			SOCKET sock = -1);
		Status receive(char* destination, const uint32_t& numberBytes, uint32_t& bytesRecived);
		Status send(std::shared_ptr<Packet> packet);
		Status receive(std::shared_ptr<Packet> packet);
	};
}