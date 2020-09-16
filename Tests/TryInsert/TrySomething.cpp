#include <iostream>
#include "MySQL_Client.h"
#include "MySQL_Impl.h"
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

//New
#include "Network.hpp"
#include "Packet.hpp"
#include "Server.hpp"
#include "Client.hpp"

using namespace std;

std::shared_ptr<swl::TCPServer> Server = std::make_shared<swl::TCPServer>();
shared_ptr<swl::TCPClient> Client = make_shared<swl::TCPClient>();

#define PORT 25566 //20675
#define IP swl::IPEndpoint::getLocalAddress()//("188.210.240.246")
int main()
{
	if (!swl::Network::initialize()) return -1;

	//size_t Choice = 0;
	//printf("Please enter one number:\n0 - Only Client\nMore Than 1 - Only Sever + MYSQL... ");
	//std::cin >> Choice;

	//if (Choice > 0)
	//{
	//	Server->run(IP, PORT);
	//	ToDo("Add Answer About Success Or Not Request (Also Add To Packet Who Was It Or Just ID)");
	//}
	json Message =
	{
		{"header",
			{
				{ "_s",1}, // Settings
				{"_t",0}, // Was 2 // Type Of Packet
				{"_R","ALL"} // ID Recipient
			}
		},
		{"data",
			{
		// The Property Of Following Data
		{"_i","trgffdsfh"}, // Id Of Packet (Needs To Be In MD5)
		{"_o",500}, // Orig Size To Decompress (If Was Decompressed)

		{"body", // All Data Is Here
			{
				{"_0","Hello!"},
				//{"_0","Login: PBAX"},
				//{"_1","Pass: _SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
			}
		}
	}
}
	},
		MySQL_Request =
	{
		{"header",
			{
				{ "_s",0}, // Settings
				{"_t",2}, // Was 2 // Type Of Packet
				{"_R",0} // ID Recipient
			}
		},
		{"data",
			{
		// The Property Of Following Data
		{"_i","nvchjfgjhfgf"}, // Id Of Packet (Needs To Be In MD5)
		{"_o",500}, // Orig Size To Decompress (If Was Decompressed)

		{"body", // All Data Is Here
			{
				{"_0","PBAX"},
				{"_1","_SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
			}
		}
	}
}
	},
		Answer_Request =
	{
		{"header",
			{
				{ "_s",0}, // Settings
				{"_t",3}, // Was 2 // Type Of Packet
				{"_R",0} // ID Recipient
			}
		},
		{"data",
			{
		// The Property Of Following Data
		{"_i","fewdadzff"}, // Id Of Packet (Needs To Be In MD5)
		{"_o",0}, // Orig Size To Decompress (If Was Decompressed)

		{"body", // All Data Is Here
			{
				{"_0","OK"},
				//{"_0","PBAX"},
				//{"_1","_SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
			}
		}
	}
}
	};

	//if (Choice > 0)
	//{
	//	std::thread([&]()
	//	{
	//		uint32_t id;
	//		std::string temp;
	//		std::shared_ptr<swl::Packet> CameUpPacket = std::make_shared<swl::Packet>();
	//		while (Server->isWork())
	//		{
	//			//if (Client->isConnected())
	//			//{
	//			//	packet->clear();
	//			//	packet->FillIn(swl::Packet::Header(swl::Packet::Type::MySQL, 0), MySQL_Request);
	//			//	Client->send(packet, 0);
	//			//	size_t id = 0;
	//			//	string temp;
	//			//	if (Client->getSocket()->receive(packet) == swl::Socket::Status::Done && packet->operator bool())
	//			//	{
	//			//		temp = packet->ToString();
	//			//		if (!temp.empty())
	//			//			printf("\nFrom Client:\n%s", temp.c_str());
	//			//	}
	//			//	this_thread::sleep_for(10ms);
	//			//	CameUpPacket = Client->getLastPacket(id);
	//			//	if (CameUpPacket && CameUpPacket->operator bool())
	//			//	{
	//			//		packet->clear();
	//			//		temp = CameUpPacket->ToString();
	//			//		printf("\nSent Some Data: %s", temp.c_str());
	//			//	}
	//			//}
	//			//else
	//			//	Client->connect(IP, PORT);
	//		}
	//	}).join();
	//	Server->stop();
	//}
	//else
	//{
	std::thread([&]()
	{
		bool IsLogging = false;
		size_t ID = 0;
		std::string Login, Pass, temp;
		while (true)
		{
			// To Do A MySQL Login And Password If It's Correctly Then We'll be able to move forward
			if (!Client->isConnected())
				Client->connect(IP, PORT);
			while (!IsLogging)
			{
				printf("Hello, you need just log in this System\nEnter Your Login Here: ");
				std::cin >> Login;
				printf("\nOK, Almost done\nEnter Your Password Here: ");
				std::cin >> Pass;

				system("cls");
				printf("\nWe're trying to verify your information... just wait!\n");

				if (!Login.empty() && !Pass.empty())
				{
					std::shared_ptr<swl::Packet> NewPacket = std::make_shared<swl::Packet>();
					MySQL_Request["data"].at("body").at("_0") = Login;
					MySQL_Request["data"].at("body").at("_1") = Pass;
					NewPacket->FillIn(swl::Packet::Header(swl::Packet::Type::MySQL, 0), MySQL_Request);
					Client->send(NewPacket, 0);
					NewPacket.~shared_ptr();

					while (true)
					{
						if (!Client->isConnected())
						{
							printf("Something is wrong with Connect To Server, try again later!\n");
							return;
						}

						swl::Packet packet = Client->getLastPacket(ID);
						if (packet && packet.operator bool() &&
							packet.getHeader().type == swl::Packet::Type::Answer)
						{
							json Hash = json::parse(packet.ToString());
							if (packet.getHeader().type == swl::Packet::Answer && !Hash.empty() &&
								Hash["data"]["body"]["_0"].get<std::string>() == "OK")
							{
								system("cls");
								printf("Here we go!\n");
								printf("\nTry to type something interesting!\n");
								IsLogging = true;
								break;
							}
							else
							{
								system("cls");
								printf("Something is wrong, try again later!\n");
							}
						}
					}
				}
			}

			swl::Packet packet = swl::Packet();
			//while (true)
			//{
			//if (temp.empty())
			//{
			if (!Client->isConnected())
			{
				system("cls");
				printf("Something is wrong with Connect To Server, try again later!\n");
				return;
			}

			packet = Client->getLastPacket(ID);
			if (packet.operator bool() && packet.getHeader().type == swl::Packet::Type::Chat)
			{
				temp = json::parse(packet.ToString())["data"].at("body").at("_0").get<std::string>();
				if (!temp.empty())
					printf("\nFrom Client: %s\n", temp.c_str());
			}

			std::cin >> temp;

			if (!temp.empty())
			{
				Message["data"].at("body").at("_0") = temp;
				packet.FillIn(swl::Packet::Header(swl::Packet::Type::Chat, 0), Message);
				Client->send(std::make_shared<swl::Packet>(packet), 0);
				temp.clear();
			}
				//}
			//}

			this_thread::sleep_for(10ms);
		}
	}).join();
	if (Client->isConnected())
		Client->disconnect();
	//	}
}