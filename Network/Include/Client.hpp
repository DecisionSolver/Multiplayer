#pragma once
#include <cstdint>
#include <queue>
#include <thread>
#include "IPEndpoint.hpp"
#include "TCPSocket.hpp"
#include "UDPSocket.hpp"
#if defined (COMPRESS_PACKETS)
	#include "ZipPacket.hpp"
#else
	#include "Packet.hpp"
#endif

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))
namespace swl
{
	class Client
	{
	public:
		Client();
		virtual ~Client();
		virtual Socket::Status connect(const IPEndpoint& ip, const uint16_t& port) = 0;
		virtual void disconnect() = 0;
		virtual void send(Packet& packet, uint32_t id) = 0;
		void setSettingsSend(const bool& encrypt, const bool& zip);
#if defined (COMPRESS_PACKETS)
		ZipPacket getLastPacket(uint32_t& id);
#else
		Packet getLastPacket(uint32_t& id);
#endif
		bool isConnected() const;
	protected:
		bool encrypt = false;
		bool zip = false;
		bool connection;
		uint32_t packetId;
#if defined (COMPRESS_PACKETS)
		std::queue<std::pair<ZipPacket, uint32_t>> packets;
#else
		std::queue<std::pair<Packet, uint32_t>> packets;
#endif
	};
	class TCPClient : public Client
	{
	public:
		TCPClient();
		~TCPClient() override;
		Socket::Status connect(const IPEndpoint& ip, const uint16_t& port) override;
		void disconnect() override;
		void send(Packet& packet, uint32_t id) override;
		TCPSocket& getSocket();
	private:
		TCPSocket socket;
	};
	class UDPClient : public Client
	{
	public:
		UDPClient();
		~UDPClient() override;
		Socket::Status connect(const IPEndpoint& NewIP, const uint16_t& NewPort) override;
		void disconnect() override;
		void send(Packet& packet, uint32_t id) override;
		UDPSocket& getSocket();
	private:
		IPEndpoint ip;
		uint16_t port;
		UDPSocket socket;
	};
}