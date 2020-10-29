#include <iostream>
#include "MySQL/MySQL_Client.h"
#include "MySQL/MySQL_Impl.h"
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include "Packet.hpp"
#include "Client.hpp"

#include <conio.h>
#include <File.hpp>

using namespace std;
using namespace net;

#define PORT 1234//17272
#define IP swl::IPEndpoint("127.0.0.1")//swl::IPEndpoint("2.tcp.ngrok.io")

std::mutex m;
std::condition_variable cv;
string Login, Pass;

int main()
{
	setlocale(LC_ALL, "Russian");
	shared_ptr<Client> Client = make_shared<net::Client>(IP, PORT);
	thread([&]()
	{
		bool IsLogging = false;
		size_t ID = 0;
		string temp;
		swl::FileTransfer File = swl::FileTransfer();
	
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
				if (!Client->IsRunning())
				{
					Client->Connect(IP, PORT);
					Client->StartSystem([&](Connection::SharedPtr Connection)
					{
						if (Connection && !Connection->GetApproved())
						{
							swl::Packet packet = swl::Packet();
							json pack = packet.CreateMySQL();
							pack["data"].at("body").at("_0") = Login;
							pack["data"].at("body").at("_1") = Pass;
							packet.FillIn(swl::Packet::Header(swl::Packet::Type::MySQL), pack);
							Connection->Send(packet);
							packet.clear();

							Connection->GetPacket(packet, swl::Packet::Type::MySQL);

							if (packet)
							{
								pack = json::parse(packet.getData());
								if (!pack.empty() && pack["_0"] == "OK")
								{
									OutputDebugStringA("\nMultiplayer::SWL (Client connected)\n");
									return; // Success
								}
								else if (!pack.empty() && pack["_0"] == "NotFound")
								{
									OutputDebugStringA("\nMultiplayer::SWL ERROR (Incorrect Login Or Password)\n");
									Connection->Stop(); // Failed!
									throw std::exception("Incorrect Login Or Password!!!");
									return;
								}
							}
						}
					});
				}
				std::unique_lock<std::mutex> lk(m);
				cv.wait(lk, [&] { return !Client->GetAllConnections().empty(); });
				if (Client->GetAllConnections().back()->IsConnected())
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
					exit(-1);
				}
			}
		}

		if (!IsLogging) exit(-1);

		auto connection = Client->GetAllConnections().back();

		while (Client->IsRunning() || connection->IsConnected())
		{
			if (!Client->IsRunning())
			{
				system("cls");
				printf("Something is wrong with Connect To Server, try again later!\n");
				return;
			}
			
			swl::Packet packet = swl::Packet();
			connection->GetPacket(packet, swl::Packet::Type::Chat);

			string temp;
			if (packet)
			{
				temp = json::parse(packet.getData())["_0"].get<string>();
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
					// Take One Of File From $(ProjectDir) -- (It's $(ProjectDir)Temp) And Transfer To ALL To Server
					//if (temp.find("/StartTransfer") != std::string::npos)
					//{
					//	packet.clear();
					//	if (File.SeparateFileIntoPackets("TrySomething.cpp")) // If It's Done Then Send File
					//		File.Worker(Client);

					//	temp.clear();
					//	system("cls");
					//}
					//else
					{
						packet.clear();
						json data = packet.CreateMessage();
						data["data"]["body"]["_0"] = temp;
						packet.FillIn(swl::Packet::Header(swl::Packet::Type::Chat, 0), data);
						connection->Send(packet);
						temp.clear();
					}
				}
			}

			this_thread::sleep_for(10ms);
		}
	}).join();
	Client->StopSystem();
}