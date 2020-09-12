#include "TCPSocket.hpp"

namespace swl
{
	TCPSocket::TCPSocket() : Socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
	{
		char value = 1;
		setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &value, 1);
	}
	TCPSocket::TCPSocket(SOCKET& handle) : Socket{ handle }
	{
		char value = 1;
		setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &value, 1);
	}
	TCPSocket::~TCPSocket()
	{

	}
	Socket::Status TCPSocket::listen(const int& backlog)
	{
		if (::listen(handle, backlog))
			return Status::Error;
		return Status::Done;
	}
	Socket::Status TCPSocket::accept(TCPSocket& socket)
	{
		sockaddr_in addr{};
		int len = sizeof(addr);
		SOCKET acceptedHandle = ::WSAAccept(handle, (sockaddr*)(&addr), &len, nullptr, 0);
		if (acceptedHandle == INVALID_SOCKET)
			return getErrorStatus();
		
		socket = TCPSocket(acceptedHandle);
		return Status::Done;
	}
	Socket::Status TCPSocket::connect(const IPEndpoint& ip, const uint16_t& port)
	{
		sockaddr_in *addr = new sockaddr_in();
		addr->sin_family = AF_INET;
		addr->sin_addr.S_un.S_addr = ip.toInteger();
		addr->sin_port = htons(port);
		if (::bind(handle, (sockaddr*)addr, sizeof(sockaddr_in)))
		if (::WSAConnect(handle, (sockaddr*)addr, sizeof(sockaddr_in), nullptr, nullptr, nullptr, nullptr))
			return getErrorStatus();
		return Status::Done;
	}
	Socket::Status TCPSocket::send(const void* data, const uint32_t& numberBytes, uint32_t& bytesSent)
	{
		bytesSent = ::send(handle, (const char*)data, numberBytes, 0);
		if (bytesSent > 2147483647)
			return getErrorStatus();
		return Status::Done;
	}
	Socket::Status TCPSocket::sendAll(const void* data, const uint32_t& numberBytes)
	{
		uint32_t totalBytesSent = 0;
		while (totalBytesSent < numberBytes)
		{
			uint32_t bytesRemainig = numberBytes - totalBytesSent;
			uint32_t bytesSent = 0;
			char* bufferOffset = (char*)data + totalBytesSent;
			Socket::Status status = send(bufferOffset, bytesRemainig, bytesSent);
			totalBytesSent += bytesSent;
			if (status != Status::Done)
				return getErrorStatus();
		}
		return Status::Done;
	}
	Socket::Status TCPSocket::receive(void* destination, const uint32_t& numberBytes, uint32_t& bytesRecived)
	{
		bytesRecived = ::recv(handle, (char*)destination, numberBytes, 0);
		if (bytesRecived > 2147483647)
			return getErrorStatus();
		return Status::Done;
	}
	Socket::Status TCPSocket::receiveAll(void* destination, const uint32_t& numberBytes)
	{
		uint32_t totalBytesReceived = 0;
		while (totalBytesReceived < numberBytes)
		{
			uint32_t bytesRemainig = numberBytes - totalBytesReceived;
			uint32_t bytesReceived = 0;
			char* bufferOffset = (char*)destination + totalBytesReceived;
			Socket::Status status = receive(bufferOffset, bytesRemainig, bytesReceived);
			totalBytesReceived += bytesReceived;
			if (status != Status::Done)
				return getErrorStatus();
		}
		return Status::Done;
	}
	Socket::Status TCPSocket::send(Packet& packet)
	{
		//ToDo
		uint32_t packetSize = 0;
		const void* data = packet.onSend(packetSize);
		packetSize = htonl(packetSize);
		if (sendAll((const void*)&packetSize, sizeof(uint32_t)))
			return getErrorStatus();
		packetSize = ntohl(packetSize);
		if (sendAll(data, packetSize))
			return getErrorStatus();
		return Status::Done;
	}
	Socket::Status TCPSocket::receive(Packet& packet)
	{
		packet.clear();
		uint32_t packetSize = 0;
		//ToDo("Think it over!");
		if (receiveAll((void*)&packetSize, sizeof(uint32_t)))
			return getErrorStatus();
		packetSize = ntohl(packetSize);
		char* data = new char[packetSize];
		if (receiveAll((void*)data, packetSize))
		{
			delete[] data;
			return getErrorStatus();
		}
		packet.onReceive(data, packetSize);
		delete[] data;
		return Status::Done;
	}
}