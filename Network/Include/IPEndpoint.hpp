#pragma once
#include "pch.h"
#include <asio.hpp>
using asio::ip::tcp;

namespace network
{
	class IPEndpoint
	{
	public:
		IPEndpoint();
		IPEndpoint(const std::string& NewIP);
		IPEndpoint(const uint32_t& NewIP);
		IPEndpoint(const in_addr& addr);
		~IPEndpoint();
		std::string toString() const;
		uint32_t toInteger() const;
		static IPEndpoint getLocalAddress();
		static sockaddr_in CreateAddress(const uint32_t& ip, const uint16_t& port);
		bool operator == (const IPEndpoint& CompareIP);
		bool operator != (const IPEndpoint& CompareIP);
	private:
		in_addr ip;
	};
}
