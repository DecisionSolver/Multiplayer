#include "pch.h"
#include "Client.hpp"

namespace net
{
	bool Client::Connect(const network::IPEndpoint& ip, const USHORT& port)
	{
		SetIP(ip.toString());
		SetPort(port);

		std::this_thread::sleep_for(200ms);
		return ConnectToServer();
	}

	void Client::Disconnect()
	{
		if (!one_connection) return;
		one_connection->Stop();
		one_connection.reset();
	}
}