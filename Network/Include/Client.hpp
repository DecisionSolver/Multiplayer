#pragma once
#include <cstdint>
#include <queue>
#include <thread>
#include "IPEndpoint.hpp"
#include "TCPSocket.hpp"
#include "UDPSocket.hpp"
#include "Packet.hpp"
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"
#include "File.hpp"

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))
namespace swl
{
	// for convenience
	using json = nlohmann::json;

	class Client
	{
	public:
		Client() {}
		virtual ~Client() {}
		
		virtual Socket::Status connect(const IPEndpoint& ip, const uint16_t& port,
			const std::string Login, const std::string Password) = 0;
		virtual void disconnect() = 0;
		
		virtual void send(Packet packet) = 0;
		void setSettingsSend(const bool& encrypt, const bool& zip);
		
		Packet getLastPacket(uint32_t& id); //, Packet::Type TypePacket
		Packet getLastPacket(uint32_t& id, Packet::Type TypePacket);

		bool isConnected() const;
	protected:
		bool encrypt = false, zip = false, connection = false;
		uint32_t packetId = 0u;
		std::vector<std::pair<Packet, uint32_t>> packets;
	};
	class TCPClient : public Client
	{
	public:
		TCPClient() {}
		~TCPClient() override;
		Socket::Status connect(const IPEndpoint& ip, const uint16_t& port,
			const std::string Login, const std::string Password) override;
		void disconnect() override;
		// Send Packet To Server
		void send(Packet packet) override;
		TCPSocket& getSocket();
	private:
		TCPSocket socket;
	};
	class UDPClient : public Client
	{
	public:
		UDPClient() {}
		~UDPClient() override;
		Socket::Status connect(const IPEndpoint& NewIP, const uint16_t& NewPort,
			const std::string Login, const std::string Password) override;
		void disconnect() override;
		// Send Packet To Server
		void send(Packet packet) override;
		UDPSocket& getSocket();
	private:
		IPEndpoint ip;
		uint16_t port;
		UDPSocket socket;
	};
}