#pragma once
#include "pch.h"
#include "ConnMan.h"
#include "IPEndpoint.hpp"

namespace net
{
	class Client: public ConnectionManager
	{
	public:
		Client(size_t numThreads = 2): ConnectionManager(ConnectionManager::TypeWorking::Client, TypeProtocol::TCP,
			"127.0.0.1", 0, numThreads) {}

		Client(network::IPEndpoint IP, TypeProtocol _Proto, USHORT Port, size_t numThreads = 2):
		ConnectionManager(ConnectionManager::TypeWorking::Client, _Proto, IP.toString(), Port, numThreads) {}

		~Client() {}
		
		bool Connect(const network::IPEndpoint& ip, const USHORT& port);
		void Disconnect();
	};
}