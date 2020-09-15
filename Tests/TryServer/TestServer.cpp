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

//// for convenience
//using json = nlohmann::json;
//using namespace std;
//
//shared_ptr<swl::TCPClient> Client = make_shared<swl::TCPClient>();
//shared_ptr<swl::TCPServer> Server = make_shared<swl::TCPServer>();
//shared_ptr<mysql::MYSQLCLIENT> User;
//
//#define PORT 25565 //20675
//#define IP swl::IPEndpoint::getLocalAddress()//("188.210.240.246")
//int main()
//{
//	swl::Network::initialize();
//	swl::Packet packet;
//
//	size_t Choice = 0;
//	printf("Please enter one number:\n0 - Only Client\nMore Than 1 - Only Sever + MYSQL... ");
//	std::cin >> Choice;
//
//		Server->run(IP, PORT);
//	json Message =
//	{
//		{"header",
//			{
//				{ "_s",1}, // Settings
//				{"_t",0}, // Was 2 // Type Of Packet
//				{"_R",0} // ID Recipient
//			}
//		},
//		{"data",
//			{
//		// The Property Of Following Data
//		{"_i","trgffdsfh"}, // Id Of Packet (Needs To Be In MD5)
//		{"_o",500}, // Orig Size To Decompress (If Was Decompressed)
//
//		{"body", // All Data Is Here
//			{
//				{"_0","Hello!"},
//				//{"_0","Login: PBAX"},
//				//{"_1","Pass: _SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
//			}
//		}
//	}
//}
//	},
//		MySQL_Request =
//	{
//		{"header",
//			{
//				{ "_s",0}, // Settings
//				{"_t",2}, // Was 2 // Type Of Packet
//				{"_R",0} // ID Recipient
//			}
//		},
//		{"data",
//			{
//		// The Property Of Following Data
//		{"_i","nvchjfgjhfgf"}, // Id Of Packet (Needs To Be In MD5)
//		{"_o",500}, // Orig Size To Decompress (If Was Decompressed)
//
//		{"body", // All Data Is Here
//			{
//				{"_0","PBAX"},
//				{"_1","_SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
//			}
//		}
//	}
//}
//	},
//		Answer_Request =
//	{
//		{"header",
//			{
//				{ "_s",0}, // Settings
//				{"_t",3}, // Was 2 // Type Of Packet
//				{"_R",0} // ID Recipient
//			}
//		},
//		{"data",
//			{
//		// The Property Of Following Data
//		{"_i","fewdadzff"}, // Id Of Packet (Needs To Be In MD5)
//		{"_o",0}, // Orig Size To Decompress (If Was Decompressed)
//
//		{"body", // All Data Is Here
//			{
//				{"_0","OK"},
//				//{"_0","PBAX"},
//				//{"_1","_SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
//			}
//		}
//	}
//}
//	};

using namespace swl;
TCPServer server;

int main()
{
	setlocale(LC_ALL, "UTF8");
	if (Network::initialize())
	{
		std::string IP = IPEndpoint::getLocalAddress().toString(), NewIP, buffer;
		uint16_t Port = 25566, NewPort = 0;

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
///*						case swl::Packet::Type::MySQL:
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