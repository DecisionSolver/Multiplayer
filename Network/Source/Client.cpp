#include "Client.hpp"

ToDo("Add Catcher Message And Store Them In Massive (With Thread)");
namespace swl
{
	void Client::setSettingsSend(const bool& _encrypt, const bool& _zip)
	{
		encrypt = _encrypt;
		zip = _zip;
	}
	Packet Client::getLastPacket(uint32_t& id)
	{
		Packet packet = Packet();
		id = 0;
		if (!packets.empty())
		{
			packet = packets.front().first;
			id = packets.front().second;
			packets.pop_front();
		}
		return packet;
	}

	bool Client::isConnected() const
	{
		return connection;
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
		else
		{
			socket.close();
			socket = TCPSocket();
		}
		if (socket.connect(ip, port))
			return Socket::Error;
		connection = true;
		std::thread([&]()
		{
			std::shared_ptr<Packet> packet = std::make_shared<Packet>();
			uint32_t id = 0;
			Socket::Status status;
			while (connection)
			{
				ToDo("Reformatting everything is here!");
				//status = socket.receiveAll((void*)&id, 4);
				status = socket.receive(packet);
				if (status == Socket::Disconnected)
				{
					connection = false;
					break;
				}
				packets.push_back(std::make_pair(*packet, id));
			}
		}).detach();
		return Socket::Done;
	}
	void TCPClient::disconnect()
	{
		connection = false;
		if (socket.getHandle() != INVALID_SOCKET)
			socket.close();
	}
	void TCPClient::send(std::shared_ptr<Packet> packet, uint32_t id)
	{
		if (!connection) return;
		swl::Socket::Status ST;
		//if ((ST = socket.sendAll((const void*)&id, 4)) != swl::Socket::Status::Done)
		//	printf(("Status: "+ std::to_string(ST) + std::string(__FILE__) + "\n" + (__FUNCTION__) +
		//		" on line " + std::to_string(__LINE__)).c_str());
		if ((ST = socket.send(packet)) != swl::Socket::Status::Done)
			printf(("Status: " + std::to_string(ST) + std::string(__FILE__) + "\n" + (__FUNCTION__) +
				" on line " + std::to_string(__LINE__)).c_str());
	}
	TCPSocket& TCPClient::getSocket()
	{
		return socket;
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
		std::shared_ptr<Packet> pconn = std::make_shared<Packet>(), packet = std::make_shared<Packet>();
		//socket.sendAll((void*)&conn, 4, ip, port);
		socket.send(pconn, ip, port);
		std::thread([&]()
		{
			uint32_t id = 0u;
			IPEndpoint ip = IPEndpoint();
			uint16_t port = 0;
			Socket::Status status;
			while (connection)
			{
				status = socket.receive(packet, ip, port);
				if (status != Socket::Done)
				{
					connection = false;
					break;
				}
				packets.push_back(std::make_pair(*packet, id));
			}
		}).detach();
		return Socket::Done;
	}
	void UDPClient::disconnect()
	{
		std::shared_ptr<swl::Packet> packet = std::make_shared<Packet>();
		send(packet, 0x7FFFFFFF);
		connection = false;
		socket.close();
	}
	void UDPClient::send(std::shared_ptr<Packet> packet, uint32_t id)
	{
		if (!connection) return;
		//socket.sendAll((const void*)&id, 4, ip, port);
		socket.send(packet, ip, port);
	}
	UDPSocket& UDPClient::getSocket()
	{
		return socket;
	}
}