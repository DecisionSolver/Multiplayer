#pragma once
#include <WinSock2.h>
#include "IPEndpoint.hpp"
#include "Packet.hpp"

namespace swl
{
	class Socket
	{
	public:
		enum Status
		{
			Done,
			NotReady,
			Partial,
			Disconnected,
			Error
		};
		enum Option
		{
			TCPNoDelay,
			SOLinger
		};
		Socket(SOCKET NewHandle);
		virtual ~Socket();
		Status bind(const IPEndpoint& ip, const uint16_t& port);
		Status close();
		SOCKET getHandle();
		Status setBlocking(const bool& blocking);
	protected:
		SOCKET handle;
		Status getErrorStatus();
	};
}