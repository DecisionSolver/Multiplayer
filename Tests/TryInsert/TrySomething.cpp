#include <iostream>
#include "MySQL_Client.h"
#include "MySQL_Impl.h"
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include "Network.hpp"
#include "Packet.hpp"
#include "Server.hpp"
#include "Client.hpp"

//New
#include <conio.h>

using namespace std;

shared_ptr<swl::TCPServer> Server = make_shared<swl::TCPServer>();
shared_ptr<swl::TCPClient> Client = make_shared<swl::TCPClient>();

#define PORT 12980//12548
#define IP swl::IPEndpoint/*::getLocalAddress()*/("2.tcp.ngrok.io")
int main()
{
	if (!swl::Network::initialize()) return -1;

	//size_t Choice = 0;
	//printf("Please enter one number:\n0 - Only Client\nMore Than 1 - Only Sever + MYSQL... ");
	//cin >> Choice;

	//if (Choice > 0)
	//{
	//	Server->run(IP, PORT);
	//}

	//if (Choice > 0)
	//{
	//	thread([&]()
	//	{
	//		uint32_t id;
	//		string temp;
	//		shared_ptr<swl::Packet> CameUpPacket = make_shared<swl::Packet>();
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
	thread([&]()
	{
		bool IsLogging = false;
		size_t ID = 0;
		string Login, Pass, temp;
		while (true)
		{
			// To Do A MySQL Login And Password If It's Correctly Then We'll be able to move forward
			while (!IsLogging)
			{
				printf("Hello, you need just log in this System\nEnter Your Login Here: ");
				cin >> Login;
				printf("\nOK, Almost done\nEnter Your Password Here: ");
				cin >> Pass;

				system("cls");
				printf("\nWe're trying to verify your information... just wait!\n");

				if (!Login.empty() && !Pass.empty())
				{
					if (!Client->isConnected())
					{
						if (Client->connect(IP, PORT, Login, Pass) == swl::Socket::Status::Done)
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

			packet = Client->getLastPacket(ID, swl::Packet::Type::Chat);
			if (packet.operator bool())
			{
				temp = json::parse(packet.ToString())["data"].at("body").at("_0").get<string>();
				if (!temp.empty())
					printf("\nFrom Client: %s\n", temp.c_str());
			}

			if (kbhit())
			{
				cout << endl;
				temp.push_back(getch());
				cout << temp.c_str() << endl;
				if (!temp.empty() && GetAsyncKeyState(VK_RETURN))
				{
					packet.clear();
					packet.FillIn(swl::Packet::Header(swl::Packet::Type::Chat, 0),
						(packet.CreateMessage()["data"].at("body").at("_0") = temp));
					Client->send(packet);
					temp.clear();
					system("cls");
				}
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