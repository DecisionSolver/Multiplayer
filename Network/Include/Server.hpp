#pragma once
#include <cstdint>
#include <vector>
#include <tuple>
#include "TCPSocket.hpp"
#include "UDPSocket.hpp"
#include "Client.hpp"
#include "SocketSelector.hpp"

namespace swl
{
	class Server
	{
	public:
		Server();
		virtual ~Server() {}
		virtual void run(const IPEndpoint& ip, uint16_t port) = 0;
		bool isWork() const;
		IPEndpoint getServerIP() { return IP; }
		uint16_t getServerPort() { return Port; }
		virtual void stop() = 0;
	protected:
		std::thread main;
		bool work = false;
		SocketSelector selector;
		IPEndpoint IP = IPEndpoint("");
		uint16_t Port = 0;
	};

	class TCPServer : public Server
	{
	public:
		TCPServer();
		~TCPServer() override;
		void run(const IPEndpoint& ip, uint16_t port) override;
		void core();
		void stop() override;
		void SendTo(SOCKET sock, const std::shared_ptr<Packet> packet);
		std::vector<std::pair<TCPSocket, uint32_t>> getClients() { return clients; }
	private:
		TCPSocket socket;
		std::vector<std::pair<TCPSocket, uint32_t>> clients;
	};

	class UDPServer : public Server
	{
	public:
		UDPServer();
		~UDPServer() override;
		void run(const IPEndpoint& ip, uint16_t port) override;
		void stop() override;
		//void SendTo(const size_t id, Packet packet);
	private:
		UDPSocket socket;
		std::vector<std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>> clients;
		//std::map<std::pair<IPEndpoint, uint16_t>, std::pair<UDPClient, uint32_t>> clients;
	};
}