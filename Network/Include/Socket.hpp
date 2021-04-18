#pragma once
#include "pch.h"
#include "IPEndpoint.hpp"

namespace network
{
	class Socket: public std::enable_shared_from_this<Socket>
	{
	public:
		Socket(asio::io_service &io_service, tcp::socket handle);
		virtual ~Socket() {}
		void close();
		tcp::socket &getSocket();
	protected:
		tcp::socket handle;
		asio::io_service &io_service;
	};
}