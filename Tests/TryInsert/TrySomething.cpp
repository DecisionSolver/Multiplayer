#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include <conio.h>

using namespace std;
#include "MySQL/MySQL_Client.h"
#include "Client.hpp"
#include "Packet.hpp"

shared_ptr<mysql::Client> DB = make_shared<mysql::Client>();
vector<shared_ptr<net::Client>> Users;

#define IP network::IPEndpoint("127.0.0.1")
#define PORT 20675

bool UseRepeater = false, UseClear = false;
void ConnectFunc(string Login, string Pass)
{
	if (Login.empty() || Pass.empty()) return;

	Users.push_back(make_shared<net::Client>(network::IPEndpoint(""), ConnectionManager::TypeProtocol::TCP, 0));
#if defined(USE_SSL)
	Users.back()->Set_Cert_Chain("keys/rootca.crt");
	Users.back()->Set_Cert_RSA_Private("keys/user.key");
#endif
	if (Users.back()->Connect(IP, PORT))
	{
		Users.back()->StartSystem();

		// Create MySQL Packet That We're Connection To
		network::Packet Answer = network::Packet();
		Answer.CreatePacket(network::Packet::Type::MySQL, true, { { "_0", Login },
			{ "_1", md5_from_buffer(Pass) } });

		Users.back()->Send(Answer);
	}
	else
	{
		system("cls");
#if defined(HAS_LOGGER)
		Logger_Error_F("User: %s, didn't connect to %s", Login.c_str(), IP.toString().c_str());
#endif
	}
}
void GetPacketFromThread(network::Packet packet, network::Packet::Type NeededPacket)
{
	if (packet && (packet.getHeader().type == NeededPacket) && packet.getHeader().IsAnswer)
	{
		json unparsed = packet.getData();
		size_t i = 0;
		for (size_t i = 0; i < unparsed["_0"].size(); i++)
		{
#if defined(HAS_LOGGER)
			Logger_Info_F("\tID: %i - Name: %s\n", ((int)unparsed["_1"].at(i).front().get<json::number_integer_t>()),
				unparsed["_0"].at(i).front().get<json::string_t>().c_str());
#endif
			i++;
		}
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	DB->Connect("gb_1231", "J4hKTTK-LHwU",
#if defined(_DEBUG)
		"mysql92.1gb.ru"
#else
		"192.168.1.2"
#endif
		, "gb_1231");

	int Choice = 0;
	while (true)
	{
#if defined(HAS_LOGGER)
		Logger_Info("Choice The One:\n");
		Logger_Info("\t[0] - Login Under All Users That Are Free Now\n");
		Logger_Info("\t[1] - Login Under Needed Account\n");
		Logger_Info("\t[2] - Exit\n");
	
		Logger_Info(": ");
#endif

		cin >> Choice;
		switch (Choice)
		{
		case 0:
		{
			auto AllUsers = DB->SelectValues("Local", { "*" }, { " WHERE _2 = 0" });
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
#if defined(HAS_LOGGER)
			Logger_Info("Enter Login Here: ");
			cin >> Login;
#endif
#if defined(HAS_LOGGER)
			Logger_Info("Enter Password Here: ");
			cin >> Pass;
#endif

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
#if defined(HAS_LOGGER)
			Logger_Error("Unrecognized Choice. Try Another One!\n");
#endif
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
				network::Packet packet;

				if (Users.size() == 1)
				{
					if (!UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
						packet.clear();

						OutputDebugStringA(("\n" + std::to_string(Users.front()->GetConnect()->GetCurrentPing()) + " ms\n").c_str());
					}
					while (UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
						packet.clear();

						OutputDebugStringA(("\n" + std::to_string(Users.front()->GetConnect()->GetCurrentPing()) + " ms\n").c_str());
					}
				}
				else if (Users.size() > 1)
				{
					if (!UseRepeater)
					{
						for (auto CurrentUser: Users)
						{
							if (!CurrentUser->GetConnect()) continue;
							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
							GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
							packet.clear();
						
							OutputDebugStringA(("\n" + std::to_string(CurrentUser->GetConnect()->GetCurrentPing()) + " ms\n").c_str());
						}
					}
					while (UseRepeater)
					{
						for (auto CurrentUser: Users)
						{
							if (!CurrentUser->GetConnect()) continue;
							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
							GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
							packet.clear();
							
							OutputDebugStringA(("\n" + std::to_string(CurrentUser->GetConnect()->GetCurrentPing()) + " ms\n").c_str());
						}
					}
				}

			}
		}).detach();
		while (true)
		{
			Sleep(1000);
#if defined(HAS_LOGGER)
			Logger_Info("Choice The One:\n");
			Logger_Info("\t[0] - Get Ping (ECHO)\n");
			Logger_Info("\t[1] - Send Chat Message Packet\n");
			Logger_Info("\t[2] - Send Random Coordinates Level Object\n");
			Logger_Info("\t[3] - Send \"Sound Play\" Packet (By Default It's \"01.08.16.wav\")\n");

			Logger_Info("\t[4] - Get List Online Users\n");

			Logger_Info("\t[5] - Back To Previous Menu\n");

			Logger_Info_F("\t[6] - Use Repeater Any Packets (%s - is now)\n", UseRepeater ? "ON" : "OFF");
			
			Logger_Info_F("\t[7] - Use Clear Screen After Command (%s - is now)\n", UseClear ? "ON" : "OFF");

			Logger_Info(": ");
#endif
			cin >> Choice;
			switch (Choice)
			{
			case 0:
			{
				std::thread t = std::thread([&]
				{
					network::Packet packet;
					packet.CreatePacket(network::Packet::Type::Ping, false);

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
			case 1:
			{
				Text.clear();
				system("cls");
#if defined(HAS_LOGGER)
				Logger_Info("Enter Message Here: ");
#endif
				cin >> Text;

				std::thread t = std::thread([&]
				{
					network::Packet packet;
					packet.CreatePacket(network::Packet::Type::Chat, false, { { "_0", Text + "\n" } });

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
				// Get List Of Current Game Objects Project


				break;
			}
			case 3:
			{
				Text.clear();
				system("cls");
#if defined(HAS_LOGGER)
				Logger_Info("Enter ID User Here (or '-1' To All Users That Are Online): ");
#endif
				cin >> Text;

				if (Text == "-1")
				{
					auto AllUsersID = DB->SelectValues("Local", { "_N" }, { " WHERE _N = 1" });
					vector<network::Packet> packet;

					for (auto ID: AllUsersID)
					{
						packet.push_back(network::Packet());
						packet.back().CreatePacket(network::Packet::Type::PlaySound, false,
							{
								{ "_0", ID.back().get<json::number_integer_t>() },
								{ "_1", 0.016f },
								{ "_2", "01.08.16.wav" }
							});
					}
					for (auto ThisPacket: packet)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->Send(ThisPacket);
					}
				}
				else
				{
					network::Packet packet;
					packet.CreatePacket(network::Packet::Type::PlaySound, false,
						{
							{ "_0", atoi(Text.c_str()) },
							{ "_1", 0.016f },
							{ "_2", "01.08.16.wav" }
						});

					if (Users.front()->GetConnect())
						Users.front()->GetConnect()->Send(packet);
				}
				break;
			}
			case 4:
			{
				network::Packet packet;
				packet.CreatePacket(network::Packet::Type::GetListUsersOnline, false);

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
			case 5:
			{
				break;
			}
			case 6:
			{
				UseRepeater = !UseRepeater;
				break;
			}
			case 7:
			{
				UseClear = !UseClear;
				break;
			}
			}
			if (Choice == 5)
				break;

			if (UseClear)
			{
				Sleep(1000);
				system("cls");
			}
		}
	}

	return 0;
}
