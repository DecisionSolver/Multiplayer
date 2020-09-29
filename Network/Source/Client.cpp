#include "Client.hpp"

#include <chrono>
#include <direct.h>

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
			packets.erase(packets.begin());
		}
		return packet;
	}
	Packet Client::getLastPacket(uint32_t& id, Packet::Type TypePacket)
	{
		Packet packet = Packet();
		id = 0;
		if (!packets.empty())
		{
			for (size_t i = 0; i < packets.size(); i++)
			{
				packet = packets.at(i).first;
				if (packet.getHeader().type == TypePacket)
					break;
			}
			id = packets.front().second;
			packets.erase(packets.begin());
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
	Socket::Status TCPClient::connect(const IPEndpoint& ip, const uint16_t& port,
		const std::string Login, const std::string Password)
	{
		if (connection) return Socket::Done;
		if (socket.getHandle() == INVALID_SOCKET)
			socket = TCPSocket();
		else
		{
			socket.close();
			socket = TCPSocket();
		}
		if (socket.connect(ip, port) != Socket::Done)
			return Socket::Error;
		connection = true;

		std::thread([&]()
		{
			Packet packet = Packet();
			//		Log In PART!!!
			//
			// Waiting For "OK" Answer From Server
			//
			Sleep(1500);
			Socket::Status status = socket.receive(packet);
			if (status == Socket::Disconnected ||
				status == Socket::NotReady ||
				status == Socket::Error)
			{
				connection = false;
				disconnect(); // Failed!
				return status;
			}
			if (packet && json::parse(packet.ToString())["data"]["body"]["_0"].get<std::string>() == "OK" &&
				packet.getHeader().type & (swl::Packet::Type::Answer << swl::Packet::Type::Connection))
			{
				packet.clear();
				json pack = packet.CreateMySQL();
				pack["data"].at("body").at("_0") = Login;
				pack["data"].at("body").at("_1") = Password;
				packet.FillIn(swl::Packet::Header(swl::Packet::Type::MySQL, 0), pack);
				socket.send(packet);

				status = socket.receive(packet);
				if (status == Socket::Disconnected ||
					status == Socket::NotReady ||
					status == Socket::Error)
				{
					connection = false;
					disconnect(); // Failed!
					return status;
				}

				pack = json::parse(packet.ToString());
				if (!pack.empty() && pack["data"]["body"]["_0"].get<std::string>() == "OK")
				{
					OutputDebugStringA("\nMultiplayer::SWL (Client connected)\n");
					return Socket::Done; // Success
				}
				else if (!pack.empty() && pack["data"]["body"]["_0"].get<std::string>() == "NotFound")
				{
					OutputDebugStringA("\nMultiplayer::SWL ERROR (Incorrect Login Or Password)\n");
					disconnect(); // Failed!
					throw std::exception("Incorrect Login Or Password!!!");
					return Socket::Disconnected;
				}
			}
		}).join();

		std::thread([&]()
		{
			Packet packet = Packet();
			uint32_t id = 0;
			swl::FileTransfer NewFile = swl::FileTransfer();
			while (connection)
			{
				Socket::Status status = socket.receive(packet);
				if (status == Socket::Disconnected ||
					status == Socket::NotReady ||
					status == Socket::Error)
				{
					connection = false;
					disconnect(); // Failed!
					break;
				}
				if (packet && packet.getHeader().type & (swl::Packet::Type::File))
				{
					std::string FilePath = _getcwd(nullptr, 1024);
					NewFile.Save(FilePath + "\\NewFile.cpp", packet, this);
					packet.clear(); // Delete Useless Used Packet!
				}
				if (packet)
					packets.push_back(std::make_pair(packet, id));
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
	void TCPClient::send(Packet packet)
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
	Socket::Status UDPClient::connect(const IPEndpoint& NewIP, const uint16_t& NewPort,
		const std::string Login, const std::string Password)
	{
		if (connection) return Socket::Done;
		if (socket.getHandle() == INVALID_SOCKET)
			socket = UDPSocket();
		socket.bind(IPEndpoint::getLocalAddress(), 0);
		connection = true;
		ip = NewIP;
		port = NewPort;
		//uint32_t conn = 0x7FFFFFFF;
		Packet pconn = Packet(), packet = Packet();
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
				packets.push_back(std::make_pair(packet, id));
			}
		}).detach();
		return Socket::Done;
	}
	void UDPClient::disconnect()
	{
		swl::Packet packet = Packet();
		send(packet/*, 0x7FFFFFFF*/);
		connection = false;
		socket.close();
	}
	void UDPClient::send(Packet packet/*, uint32_t id*/)
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