#include "pch.h"
#include "Client.hpp"

namespace network
{
	bool Client::Connect(const std::string &ip, const USHORT &port, const std::string &Login, const std::string &Pass)
	{
		SetIP(ip);
		SetPort(port);

		std::this_thread::sleep_for(200ms);
		if (ConnectToServer(Login, Pass))
			return true;
		else
			return false;
		return false;
	}

	void Client::Disconnect()
	{
		if (!one_connection) return;
		one_connection->Stop();
		one_connection.reset();
	}

	std::map<network::Packet::Status, std::string> Client::PacketReasons =
	{
		{
			network::Packet::Status::TimeOut_LogIn,
			"User Has Been Disconnected By Left Time Waiting For Login Packet!"
		},
		{
			network::Packet::Status::NotAllowWithoutCookie,
			"The server rejects your connection because you don't have cookie to the server, try to connect to the main server to set your cookie and try again."
		},
	};
}