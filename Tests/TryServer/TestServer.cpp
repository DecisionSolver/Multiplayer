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
using namespace swl;
#include <conio.h>

int main()
{
	setlocale(LC_ALL, "Russian");

	IPEndpoint IP = IPEndpoint(
#if defined(_DEBUG)
		"127.0.0.1"
#else
		"192.168.1.2"
#endif
	);
	uint16_t Port = 20675;
	std::shared_ptr<Server> server = std::make_shared<Server>(IP, Port);

	server->Start();

	std::thread([&]()
	{
		while (server->IsWorking() && !server->isInUpdate())
		{
			Sleep(500);
		}
	}).join();

	server->StopSystem();
	return 0;
}