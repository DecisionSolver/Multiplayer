#pragma once
#include "pch.h"

#include "Client.hpp"
#include "ConnMan.h"

namespace net
{
	class Server: public ConnectionManager
	{
	public:
		Server(const std::string &IP = "127.0.0.1", const TypeProtocol &_Proto = TypeProtocol::TCP,
			const USHORT &Port = 25565, const size_t &numThreads = 2):
			ConnectionManager(ConnectionManager::TypeWorking::Server, _Proto, IP, Port, numThreads) {}
		~Server() {}
		void Start();
		void Send(const std::string &Packet);
		void Send(const std::shared_ptr<network::Packet> &Packet);

		bool IsWorking() const { return IsRunning(); }
	private:
		void OnPacketHandler(Connection::SharedPtr connection);
	};
	class ServerFTP: public ConnectionManager
	{
	public:
		ServerFTP(const std::string &IP = "127.0.0.1", const TypeProtocol &_Proto = TypeProtocol::FTP, 
			const USHORT &Port = 21, const size_t &numThreads = 2):
			ConnectionManager(ConnectionManager::TypeWorking::Server, _Proto, IP, Port, numThreads) {}
		~ServerFTP() {}
		void Start();

		bool IsWorking() const { return IsRunning(); }
	};
}