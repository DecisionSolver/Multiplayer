#pragma once
#include <iostream>
#include <fstream>
#include <conio.h>
#include <direct.h>

#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

//New
#include "Packet.hpp"
#include "Servers.hpp"
#include "Client.hpp"

#include "ODBC/ODBC.h"

#include <boost/filesystem.hpp>
using namespace boost::filesystem;
using namespace net;
using namespace network;

USHORT Port = 20675;
std::shared_ptr<Server> This_Server = std::make_shared<Server>();
//std::shared_ptr<ServerFTP> This_Server_FTP = std::make_shared<ServerFTP>();

int main()
{
	This_Server->Set_All_Paths("keys/rootca.crt", "keys/rootca.key", "keys/dh2048.pem", "keys/user.key");

	This_Server->Start();
	//This_Server_FTP->Start();

	std::thread([&]()
	{
		while (true)
		{
			std::string Cmd;
			/*
			std::cin >> Cmd;
			if (Cmd == "disable")
			{
				CppLogger::DisablePrintAll();
				Logger_Error("Now It Doesn't Work");
			}
			if (Cmd == "enable")
			{
				CppLogger::EnablePrintAll();
				Logger_Error("Now It Works");
			}
			*/
			if ((This_Server->IsWorking() && !This_Server->isInUpdate()))
				//|| (This_Server_FTP->IsWorking() && !This_Server_FTP->isInUpdate()))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			else
			{
				Logger_Info("The System is Going Shutdown!");
				break;
			}
		}
	}).join();

	This_Server->StopSystem();
	//This_Server_FTP->StopSystem();

	return 0;
}