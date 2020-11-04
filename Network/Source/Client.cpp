#include "pch.h"
#include "Client.hpp"

namespace net
{
	void Client::Connect(const swl::IPEndpoint& ip, const uint16_t& port)
	{
		if (_IP.empty())
			SetIP(ip.toString());
		if (_Port == 0)
			SetPort(port);
		ConnectToServer();
	}

	void Client::Disconnect()
	{
		if (!one_connection) return;
		one_connection->Stop();
		one_connection.reset();
	}
}