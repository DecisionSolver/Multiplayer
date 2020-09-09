#pragma once
#include <cstdint>
#include <vector>
#include "Socket.hpp"

namespace swl
{
	class SocketSelector
	{
	public:
		enum Status
		{
			Read,
			Write,
			NotReady,
			Disconnected,
			Error
		};
		SocketSelector();
		~SocketSelector();
		void add(Socket& socket);
		void remove(Socket& socket);
		bool wait(int32_t millisecond = -1);
		SocketSelector::Status isReady(Socket& _socket);
		void clear();
	private:
		std::vector<pollfd> sockets;
	};
}