#pragma once
#include "pch.h"
#include "ConnMan.h"

namespace network
{
	class Client: public ConnectionManager
	{
	public:
		Client(size_t numThreads = 2): ConnectionManager(ConnectionManager::TypeWorking::Client, (int)TypeProtocol::TCP,
			"127.0.0.1", 0, numThreads) {}

		Client(const std::string &IP, int TypeProtocol, USHORT Port, size_t numThreads = 2):
		ConnectionManager(ConnectionManager::TypeWorking::Client, TypeProtocol, IP, Port, numThreads) {}

		~Client() = default;
		
		bool Connect(const std::string &ip, const USHORT &port, const std::string &Login = std::string(),
			const std::string &Pass = std::string());
		void Disconnect();

		// Match String Message Reasons With Server To Client
		static std::map<network::Packet::Status, std::string> PacketReasons;
	};
}