#pragma once
#include "Socket.hpp"

namespace swl
{
	class UDPSocket : public Socket
	{
	public:
		UDPSocket();
		~UDPSocket();
		Status send(const void* data, const uint32_t& numberBytes, uint32_t& bytesSent, const IPEndpoint& ip, const uint16_t& port);
		Status sendAll(const void* data, const uint32_t& numberBytes, const IPEndpoint& ip, const uint16_t& port);
		Status receive(void* destination, const uint32_t& numberBytes, uint32_t& bytesRecived, IPEndpoint& ip, uint16_t& port);
		Status receiveAll(void* destination, const uint32_t& numberBytes, IPEndpoint& ip, uint16_t& port);
		Status send(Packet& packet, const IPEndpoint& ip, const uint16_t& port);
		Status receive(Packet& packet, IPEndpoint& ip, uint16_t& port);
	};
}