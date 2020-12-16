#pragma once
#include "pch.h"

#include "Client.hpp"
#include "ConnMan.h"
#include "IPEndpoint.hpp"

namespace net
{
	class Server: public ConnectionManager
	{
	public:
		Server(swl::IPEndpoint IP = swl::IPEndpoint("127.0.0.1"), TypeProtocol _Proto = TypeProtocol::UDP,
			unsigned Port = 25565, size_t numThreads = 2):
			ConnectionManager(ConnectionManager::TypeWorking::Server, _Proto, IP.toString(), Port, numThreads) {}
		~Server() {}
		void Start();
		void Send(std::string Packet);
		
		bool IsWorking() const { return IsRunning(); }
	private:
		void OnPacketHandler(Connection::SharedPtr connection);
	};
}