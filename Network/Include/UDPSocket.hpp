#pragma once
#include "Socket.hpp"

namespace swl
{
	class UDPSocket : public Socket
	{
	public:
		UDPSocket();
		~UDPSocket() {}
		Status send(const char* data, const uint32_t& numberBytes, uint32_t& bytesSent, const IPEndpoint& ip, const uint16_t& port);
		Status sendAll(const char* data, const uint32_t& numberBytes, const IPEndpoint& ip, const uint16_t& port);
		Status receive(char* destination, const uint32_t& numberBytes, uint32_t& bytesRecived, IPEndpoint& ip, uint16_t& port);
		Status receiveAll(char* destination, const uint32_t& numberBytes, IPEndpoint& ip, uint16_t& port);
		Status send(std::shared_ptr<swl::Packet> packet, const IPEndpoint& ip, const uint16_t& port);
		Status receive(std::shared_ptr<swl::Packet> packet, IPEndpoint& ip, uint16_t& port);
	};
}