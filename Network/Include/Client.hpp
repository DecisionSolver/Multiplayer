#pragma once
#include "pch.h"
#include "ConnMan.h"

namespace net
{
	class Client: public ConnectionManager
	{
	public:
		Client(size_t numThreads = 2): ConnectionManager(ConnectionManager::TypeWorking::Client, TypeProtocol::TCP,
			"127.0.0.1", 0, numThreads) {}

		Client(const std::string &IP, TypeProtocol _Proto, USHORT Port, size_t numThreads = 2):
		ConnectionManager(ConnectionManager::TypeWorking::Client, _Proto, IP, Port, numThreads) {}

		~Client() {}
		
		bool Connect(const std::string &ip, const USHORT& port);
		void Disconnect();
	};
}