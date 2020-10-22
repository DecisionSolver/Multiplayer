#include "pch.h"
#include "Socket.hpp"

namespace swl
{
	Socket::Socket(asio::io_service &io_service, tcp::socket handle): io_service(io_service), handle(std::move(handle))
	{
	}
	void Socket::close()
	{
		if (handle.is_open())
			handle.close();
	}
	tcp::socket &Socket::getSocket()
	{
		return handle;
	}
}