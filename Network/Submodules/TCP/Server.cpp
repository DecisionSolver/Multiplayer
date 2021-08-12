#pragma once
#include <iostream>
#include <fstream>
#include "Server.hpp"
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

//New
#include "Packet.hpp"
#include "Server.hpp"
#include "Client.hpp"

using namespace net;
using namespace network;
#include <conio.h>

IPEndpoint IP = IPEndpoint(
	//#if defined(_DEBUG)
	"127.0.0.1"
	//#else
	//		"192.168.1.4"
	//#endif
);
USHORT Port = 20675;
std::shared_ptr<Server> This_Server = std::make_shared<Server>(IP, ConnectionManager::TypeProtocol::TCP, Port);

int main()
{
	setlocale(LC_ALL, "Russian");


	This_Server->Set_All_Paths("keys/rootca.crt", "keys/rootca.key", "keys/dh2048.pem");

	This_Server->Start();

	std::thread([&]()
	{
		while (This_Server->IsWorking() && !This_Server->isInUpdate())
		{
			std::this_thread::sleep_for(1ms);
		}
	}).join();

	This_Server->StopSystem();
	return 0;
}