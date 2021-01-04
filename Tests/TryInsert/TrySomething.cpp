#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include <conio.h>

using namespace std;
#include "MySQL/MySQL_Client.h"
#include "MySQL/MySQL_Impl.h"
#include "Client.hpp"
#include "Packet.hpp"

shared_ptr<mysql::Impl> DB = make_shared<mysql::Impl>();
vector<shared_ptr<net::Client>> Users;

#define IP swl::IPEndpoint("127.0.0.1")
#define PORT 20675

bool UseRepeater = false;
void ConnectFunc(string Login, string Pass)
{
	Users.push_back(make_shared<net::Client>(swl::IPEndpoint(""), ConnectionManager::TypeProtocol::TCP, 0));
	if (Users.back()->Connect(IP, PORT))
	{
		Users.back()->StartSystem();

		// Create MySQL Packet That We're Connection To
		swl::Packet Answer = swl::Packet();
		json pack = json::parse(Answer.CreateMySQL()->getData());
		pack["data"]["body"]["_0"] = Login;
		pack["data"]["body"]["_1"] = Pass;
		Answer.FillIn(swl::Packet::Header(swl::Packet::Type::MySQL), pack);

		Users.back()->Send(Answer.getData());
	}
	else
	{
		system("cls");
		printf("User: %s, didn't connect to the %s", Login.c_str(), IP.toString().c_str());
	}
}
void GetPacketFromThread(swl::Packet packet, swl::Packet::Type NeededPacket)
{
	if (packet && packet.getHeader().type == NeededPacket && packet.getHeader().IsAnswer)
	{
		json unparsed = json::parse(packet.getData());
		size_t i = 0;
		for (size_t i = 0; i < unparsed["_0"].size(); i++)
		{
			printf("\tID: %i\nName: %s\n", ((int)unparsed["_1"].at(i).front().get<json::number_integer_t>()),
				unparsed["_0"].at(i).front().get<json::string_t>().c_str());
			i++;
		}
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	DB->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
		"188.210.240.246"
#else
		"192.168.1.2"
#endif
		, "gb_z_rod2_rf");

	int Choice = 0;
	while (true)
	{
		printf("Choice The One:\n");
		printf("\t[0] - Login Under All Users That Are Free Now\n");
		printf("\t[1] - Login Under Needed Account\n");
		printf("\t[2] - Exit\n");

		printf(": ");

		cin >> Choice;
		switch (Choice)
		{
		case 0:
		{
			auto AllUsers = DB->TrySelectValues("Local", { "*" }, { " WHERE _2 = 0" });
			for (size_t i = 0; i < AllUsers["_N"].size(); i++)
			{
				ConnectFunc(AllUsers.at(i).get<json::string_t>(), AllUsers["_1"].at(i).get<json::string_t>());
			}
			break;
		}
		case 1:
		{
			string Login, Pass;
			system("cls");
			printf("Enter Login Here: ");
			cin >> Login;
			printf("Enter Password Here: ");
			cin >> Pass;

			ConnectFunc(Login, Pass);

			break;
		}
		case 2:
		{
			return 0;
		}
		default:
		{
			system("cls");
			printf("Unrecognized Choice. Try Another One!\n");
			continue;
		}
		}

		Sleep(1000);
		Choice = 0;
		string Text;

		std::thread([&]
		{
			while (!Users.empty())
			{
				Sleep(1000);
				swl::Packet packet;

				if (Users.size() == 1)
				{
					if (!UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, swl::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, swl::Packet::Type::GetListUsersOnline);
					}
					while (UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, swl::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, swl::Packet::Type::GetListUsersOnline);
					}
				}
				else if (Users.size() > 1)
				{
					if (!UseRepeater)
					{
						for (auto CurrentUser: Users)
						{
							if (!CurrentUser->GetConnect()) continue;
							CurrentUser->GetConnect()->GetPacket(packet, swl::Packet::Type::GetListUsersOnline);
							GetPacketFromThread(packet, swl::Packet::Type::GetListUsersOnline);
						}
					}
					while (UseRepeater)
					{
						for (auto CurrentUser: Users)
						{
							if (!CurrentUser->GetConnect()) continue;
							CurrentUser->GetConnect()->GetPacket(packet, swl::Packet::Type::GetListUsersOnline);
							GetPacketFromThread(packet, swl::Packet::Type::GetListUsersOnline);
						}
					}
				}
			}
		}).detach();
		while (true)
		{
			Sleep(1000);
			printf("Choice The One:\n");
			printf("\t[0] - Send Chat Message Packet\n");
			printf("\t[1] - Send Random Coordinates Level Object\n");
			printf("\t[2] - Send \"Sound Play\" Packet (By Default It's \"01.08.16.wav\")\n");

			printf("\t[3] - Get List Online Users\n");

			printf("\t[4] - Back To Previous Menu\n");

			printf("\t[5] - Use Repeater Any Packets (%s - is now)\n", UseRepeater ? "ON" : "OFF");
			
			printf(": ");
			cin >> Choice;
			switch (Choice)
			{
			case 0:
			{
				Text.clear();
				system("cls");
				printf("Enter Message Here: ");
				cin >> Text;

				std::thread t = std::thread([&]
				{
					swl::Packet packet;
					json data = json::parse(packet.CreateMessage()->getData());
					data["data"]["body"]["_0"] = Text + "\n";
					packet.FillIn(swl::Packet::Header(swl::Packet::Type::Chat), data);

					if (Users.size() == 1)
					{
						if (UseRepeater)
						{
							while (UseRepeater)
							{
								if (Users.front()->GetConnect())
									Users.front()->GetConnect()->Send(packet);
							}
						}
						else
						{
							if (Users.front()->GetConnect())
								Users.front()->GetConnect()->Send(packet);
						}
					}
					else if (Users.size() > 1)
					{
						if (UseRepeater)
						{
							while (UseRepeater)
							{
								for (auto CurrentUser: Users)
								{
									if (!CurrentUser->GetConnect()) continue;
									CurrentUser->GetConnect()->Send(packet);
								}
							}
						}
						else
						{
							for (auto CurrentUser: Users)
							{
								if (!CurrentUser->GetConnect()) continue;
								CurrentUser->GetConnect()->Send(packet);
							}
						}
					}
				});
				if (UseRepeater)
					t.detach();
				else
					t.join();

				break;
			}
			case 2:
			{
				Text.clear();
				system("cls");
				printf("Enter ID User Here (-1 Means That 'To All Users That Are Online'): ");
				cin >> Text;

				if (Text == "-1")
				{
					auto AllUsersID = DB->TrySelectValues("Local", { "_N" }, { " WHERE _N = 1" });
					vector<swl::Packet> packet;

					for (auto ID: AllUsersID)
					{
						packet.push_back(swl::Packet());
						json data = json::parse(packet.back().CreateMessage()->getData());
						data["data"]["body"]["_0"] = ID.back().get<json::number_integer_t>();
						data["data"]["body"]["_1"] = 0.016f;
						data["data"]["body"]["_2"] = "01.08.16.wav";
						packet.back().FillIn(swl::Packet::Header(swl::Packet::Type::PlaySound), data);
					}
					for (auto ThisPacket: packet)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->Send(ThisPacket);
					}
				}
				else
				{
					swl::Packet packet;
					json data = json::parse(packet.CreateMessage()->getData());
					data["data"]["body"]["_0"] = atoi(Text.c_str());
					data["data"]["body"]["_1"] = 0.016f;
					data["data"]["body"]["_2"] = "01.08.16.wav";
					packet.FillIn(swl::Packet::Header(swl::Packet::Type::PlaySound), data);
					
					if (Users.front()->GetConnect())
						Users.front()->GetConnect()->Send(packet);
				}
				break;
			}
			case 3:
			{
				swl::Packet packet;
				json pack = json::parse(packet.CreateMessage()->getData());
				pack["data"]["body"].clear();
				packet.FillIn(swl::Packet::Header(swl::Packet::Type::GetListUsersOnline), pack);

				if (Users.size() == 1)
				{
					if (!Users.front()->GetConnect()) continue;
					Users.front()->GetConnect()->Send(packet);
				}
				else if (Users.size() > 1)
				{
					for (auto CurrentUser: Users)
					{
						if (!CurrentUser->GetConnect()) continue;
						CurrentUser->GetConnect()->Send(packet);
					}
				}

				Sleep(1000);
				break;
			}
			case 4:
			{
				break;
			}
			case 5:
			{
				UseRepeater = !UseRepeater;
				break;
			}
			}
			if (Choice == 4)
				break;
		}
	}

	return 0;
}
