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
		virtual ~Server();
		virtual void run(const IPEndpoint& ip, uint16_t port) = 0;
		bool isWork() const;
		virtual void stop() = 0;
	protected:
		std::thread main;
		bool work;
		SocketSelector selector;

	};

	class TCPServer : public Server
	{
	public:
		TCPServer();
		~TCPServer() override;
		void run(const IPEndpoint& ip, uint16_t port) override;
		void core();
		void stop() override;
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
	private:
		UDPSocket socket;
		std::vector<std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>> clients;
		//std::map<std::pair<IPEndpoint, uint16_t>, std::pair<UDPClient, uint32_t>> clients;
	};
}