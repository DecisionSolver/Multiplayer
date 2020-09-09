#include "Network.hpp"
#include <iostream>

namespace swl
{
	bool Network::initialize()
	{
		WSAData wsaData;
		if (WSAStartup(514, &wsaData))
		{
			std::cerr << "Failed to start up the winsock api.\n";
			return false;
		}
		if (wsaData.wVersion != 0x0202)
		{
			std::cerr << "Error version winsock api.\n";
			return false;
		}
		return true;
	}
	void Network::shutdown()
	{
		WSACleanup();
	}
}