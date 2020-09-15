#pragma once
#include <cstdint>
#include <queue>
#include <thread>
#include "IPEndpoint.hpp"
#include "TCPSocket.hpp"
#include "UDPSocket.hpp"
#include "Packet.hpp"

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))
namespace swl
{
	class Client
	{
	public:
		Client() {}
		virtual ~Client() {}
		virtual Socket::Status connect(const IPEndpoint& ip, const uint16_t& port) = 0;
		virtual void disconnect() = 0;
		virtual void send(std::shared_ptr<Packet> packet, uint32_t id) = 0;
		void setSettingsSend(const bool& encrypt, const bool& zip);
		Packet getLastPacket(uint32_t& id);
		bool isConnected() const;
	protected:
		bool encrypt = false;
		bool zip = false;
		bool connection = false;
		uint32_t packetId = 0u;
		std::queue<std::pair<Packet, uint32_t>> packets;
	};
	class TCPClient : public Client
	{
	public:
		TCPClient() {}
		~TCPClient() override;
		Socket::Status connect(const IPEndpoint& ip, const uint16_t& port) override;
		void disconnect() override;
		void send(std::shared_ptr<Packet> packet, uint32_t id) override;
		TCPSocket& getSocket();
	private:
		TCPSocket socket;
	};
	class UDPClient : public Client
	{
	public:
		UDPClient() {}
		~UDPClient() override;
		Socket::Status connect(const IPEndpoint& NewIP, const uint16_t& NewPort) override;
		void disconnect() override;
		void send(std::shared_ptr<Packet> packet, uint32_t id) override;
		UDPSocket& getSocket();
	private:
		IPEndpoint ip;
		uint16_t port;
		UDPSocket socket;
	};
}