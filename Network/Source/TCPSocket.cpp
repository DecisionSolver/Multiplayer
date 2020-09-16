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
		{
			if (::WSAConnect(handle, (sockaddr*)addr, sizeof(sockaddr_in), nullptr, nullptr, nullptr, nullptr))
				return getErrorStatus();
		}
		else
				return getErrorStatus();

		return Status::Done;
	}
	Socket::Status TCPSocket::send(const char* data, const uint32_t& numberBytes, uint32_t& bytesSent, SOCKET sock)
	{
		const_cast<char *>(data)[numberBytes] = '\0';
		bytesSent = ::send(sock != -1 ? sock : handle, (const char*)data, numberBytes, 0);
		if (bytesSent > 2147483647)
			return getErrorStatus();

#if defined (_SERVER) && defined (_CONSOLE)
		std::cout << "\nSocket: " << (sock ? sock : handle) << " Sent:\nData: " << data
			 << ", numBytes: "<< numberBytes <<", bytesSent: " << bytesSent << "\n";
#endif
		return Status::Done;
	}
	Socket::Status TCPSocket::receive(char* destination, const uint32_t& numberBytes, uint32_t& bytesRecived)
	{
		bytesRecived = ::recv(handle, destination, numberBytes, 0);
		if (bytesRecived > 2147483647)
			return getErrorStatus();
		destination[bytesRecived] = '\0';

#if defined (_SERVER) && defined (_CONSOLE)
		std::cout << "\nServer Got New Packet:\nData: " << (const char*)destination
			<< ", numBytes: " << numberBytes << ", bytesRecived: " << bytesRecived << "\n";
#endif
		return Status::Done;
	}
	Socket::Status TCPSocket::send(std::shared_ptr<Packet> packet)
	{
		uint32_t packetSize = packet->getSize();
		//packetSize = htonl(packetSize);
		//if (sendAll((const void*)&packetSize, sizeof(uint32_t)))
		//	return getErrorStatus();
		//packetSize = ntohl(packetSize);
		if (send(packet->getData(), packetSize, packetSize))
			return getErrorStatus();
		return Status::Done;
	}
	Socket::Status TCPSocket::SendTo(SOCKET Where, std::shared_ptr<Packet> packet)
	{
		uint32_t packetSize;

		if (send(packet->onSend(packetSize), packetSize, packetSize, Where))
			return getErrorStatus();
		return Status::Done;
	}
	Socket::Status TCPSocket::receive(std::shared_ptr<Packet> packet)
	{
		packet->clear();
		uint32_t packetSize = 0;
		//ToDo("Think it over!");
		//if (receiveAll((void*)&packetSize, sizeof(uint32_t)))
		//	return getErrorStatus();
		//packetSize = ntohl(packetSize);
		char* data = new char[2048];
		if (receive(data, 2048, packetSize))
		{
			delete[] data;
			return getErrorStatus();
		}
		packet->onReceive(data, packetSize);
		delete[] data;
		return Status::Done;
	}
}