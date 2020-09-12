#include "Client.hpp"

ToDo("Add Catcher Message And Store Them In Massive (With Thread)");
namespace swl
{
	Client::Client() : packets{}, packetId{}
	{
		connection = false;
	}
	Client::~Client()
	{

	}
	void Client::setSettingsSend(const bool& _encrypt, const bool& _zip)
	{
		encrypt = _encrypt;
		zip = _zip;
	}
#if defined (COMPRESS_PACKETS)
	ZipPacket Client::getLastPacket(uint32_t& id)
	{
		ZipPacket packet;
		id = 0;
		if (!packets.empty())
		{
			packet = packets.front().first;
			id = packets.front().second;
			packets.pop();
		}
		return packet;
	}
#else
	Packet Client::getLastPacket(uint32_t& id)
	{
		Packet packet;
		id = 0;
		if (!packets.empty())
		{
			packet = packets.front().first;
			id = packets.front().second;
			packets.pop();
		}
		return packet;
	}
#endif
	bool Client::isConnected() const
	{
		return connection;
	}
	TCPClient::TCPClient() : socket{}
	{
		
	}
	TCPClient::~TCPClient()
	{
		connection = false;
		socket.close();
	}
	Socket::Status TCPClient::connect(const IPEndpoint& ip, const uint16_t& port)
	{
		if (connection) return Socket::Done;
		if (socket.getHandle() == INVALID_SOCKET)
			socket = TCPSocket();
		if (socket.connect(ip, port))
			return Socket::Error;
		connection = true;
		std::thread([&]() {
#if defined (COMPRESS_PACKETS)
			ZipPacket packet;
#else
			Packet packet;
#endif
			uint32_t id{};
			Socket::Status status;
			while (connection)
			{
				//{"data":{"id":"trgffdsfh"body:{"type":"5",data:"Login = '1', Pass = '2'"}, "isComp":"1"}}
				ToDo("Reformatting everything is here!");
				//status = socket.receiveAll((void*)&id, 4);
				status = socket.receive(packet);
				if (status == Socket::Disconnected)
				{
					connection = false;
					break;
				}
				packets.push(std::make_pair(packet, id));
			}
		}).detach();
		return Socket::Done;
	}
	void TCPClient::disconnect()
	{
		connection = false;
		socket.close();
	}
	void TCPClient::send(Packet& packet, uint32_t id)
	{
		socket.sendAll((const void*)&id, 4);
		socket.send(packet);
	}
	TCPSocket& TCPClient::getSocket()
	{
		return socket;
	}

	UDPClient::UDPClient() : socket{}
	{

	}
	UDPClient::~UDPClient()
	{
		connection = false;
		socket.close();
	}
	Socket::Status UDPClient::connect(const IPEndpoint& NewIP, const uint16_t& NewPort)
	{
		if (connection) return Socket::Done;
		if (socket.getHandle() == INVALID_SOCKET)
			socket = UDPSocket();
		socket.bind(IPEndpoint::getLocalAddress(), 0);
		connection = true;
		ip = NewIP;
		port = NewPort;
		uint32_t conn = 0x7FFFFFFF;
		Packet pconn;
		socket.sendAll((void*)&conn, 4, ip, port);
		socket.send(pconn, ip, port);
		std::thread([&]() {
#if defined (COMPRESS_PACKETS)
			ZipPacket packet;
#else
			Packet packet;
#endif
			uint32_t id{};
			IPEndpoint ip;
			uint16_t port;
			Socket::Status status;
			while (connection)
			{
				status = socket.receiveAll((void*)&id, 4, ip, port);
				status = socket.receive(packet, ip, port);
				if (status != Socket::Done)
				{
					connection = false;
					break;
				}
				packets.push(std::make_pair(packet, id));
			}
		}).detach();
		return Socket::Done;
	}
	void UDPClient::disconnect()
	{
		swl::Packet packet;
		send(packet, 0x7FFFFFFF);
		connection = false;
		socket.close();
	}
	void UDPClient::send(Packet& packet, uint32_t id)
	{
		socket.sendAll((const void*)&id, 4, ip, port);
		socket.send(packet, ip, port);
	}
	UDPSocket& UDPClient::getSocket()
	{
		return socket;
	}
}