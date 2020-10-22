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

int main()
{
	setlocale(LC_ALL, "Russian");
	IPEndpoint IP = IPEndpoint("127.0.0.1");
	uint16_t Port = 1234;
	std::shared_ptr<Server> server = std::make_shared<Server>(IP, Port);

	server->Start();

	std::thread([&]()
	{
		while (server->IsWorking())
		{
			Sleep(1000);
		}
	}).join();

	server->StopSystem();
	system("pause");
	return -1;
}