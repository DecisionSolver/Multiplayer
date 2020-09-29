#pragma once
#include <iostream>
#include <fstream>
#include "Network.hpp"
#include "SocketSelector.hpp"
#include "Server.hpp"
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

//New
#include "Network.hpp"
#include "Packet.hpp"
#include "Server.hpp"
#include "Client.hpp"

using namespace swl;
TCPServer server;

int main()
{
	setlocale(LC_ALL, "UTF8");
	if (Network::initialize())
	{
		std::string IP = IPEndpoint/*::getLocalAddress()*/("127.0.0.1").toString(), NewIP, buffer;
		uint16_t Port = 1234, NewPort = 0;

		//printf("New IP And Port (By Default It's IP == LocalAddress, Port == 25565)\n"\
		//	"Enter IP ('NO' Sets By Default Values): ");

		//std::cin >> NewIP;
		//printf("\nNew Port\n"\
		//	"Enter Port ('0' Sets By Default Values): ");
		//std::cin >> NewPort;

		server.run(IPEndpoint(NewIP.empty() ? IP : NewIP), NewPort > 0 ? NewPort : Port);
		std::thread([&]()
		{
			Packet packet;
			while (server.isWork())
			{
//				for (auto Client : server.getClients())
//				{
//					if (Client.first.receive(packet) != swl::Socket::Status::Done) continue;
//					if (packet.getSize() > 0)
//					{
//						packet >> buffer;
//						std::cout << buffer << " id=" << Client.second << std::endl;
//						packet.clear();
//						buffer = packet.ToString();
//						printf("\nSent Some Data: %s", buffer.c_str());
//						switch (packet.getHeader().type)
//						{
//						case swl::Packet::Type::Chat:
//							buffer = json::parse(buffer)["_0"].get<std::string>();
//							if (!buffer.empty())
//								printf("\n%s\n", buffer.c_str());
//							break;
//						case swl::Packet::Type::MySQL:
//						{
//							if (!buffer.empty())
//							{
//								std::string Login = json::parse(buffer)["_0"].get<std::string>(),
//									Pass = json::parse(buffer)["_1"].get<std::string>();
//
//								auto Obj = User->TrySelectValues("Local", { "_0", "_1" }, " WHERE _0 = '" + Login
//									+ "' AND _1 = '" + Pass + "'");
//								printf(("\nsize: " + to_string(Obj.size()) + "\n").c_str());
//								size_t I = 0;
//								for (size_t i = 0; i < Obj.size(); i++)
//								{
//									printf(("[" + to_string(i) + "] = " + Obj.at(i).first + "\n").c_str());
//
//									for (auto It : Obj.at(i).second)
//									{
//										printf(("\t\t[" + to_string(I) + "] = " + It + "\n").c_str());
//									}
//								}
//
//								// If Successfull Then Send It
//								if (!Obj.empty())
//								{
//									swl::Packet Answer;
//									Answer.FillIn(Answer_Request);
//									Server->SendTo(Client->getSocket().getHandle(), Answer);
//								}
//							}
//							break;
//						}*/
//						default:
//							printf(("Unknown type of packet it was: " + std::string(__FILE__) + "\n" + (__FUNCTION__) +
//								" on line " + std::to_string(__LINE__)).c_str());
//							break;
//						}
//					}
//				}
				Sleep(16);
			}
		}).join();
		server.stop();
		Network::shutdown();
	}
	system("pause");
	return -1;
}