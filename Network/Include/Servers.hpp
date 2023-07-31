#pragma once
#include "pch.h"

#include "Client.hpp"
#include "ConnMan.h"

namespace network
{
	class Server: public ConnectionManager
	{
	public:
		Server(const std::string &IP = "127.0.0.1", const int _Proto = (int)TypeProtocol::TCP,
			const USHORT &Port = 25565, const size_t &numThreads = 2):
			ConnectionManager(ConnectionManager::TypeWorking::Server, _Proto, IP, Port, numThreads) {}
		~Server() = default;
		void Start();
	private:
		void OnPacketHandler(Connection::SharedPtr connection);
	};
	class ServerFTP: public ConnectionManager
	{
	public:
		ServerFTP(const std::string &IP = "127.0.0.1", const int _Proto = (int)TypeProtocol::FTP, 
			const USHORT &Port = 21, const size_t &numThreads = 2):
			ConnectionManager(ConnectionManager::TypeWorking::Server, _Proto, IP, Port, numThreads) {}
		~ServerFTP() = default;
		void Start();
	};
}