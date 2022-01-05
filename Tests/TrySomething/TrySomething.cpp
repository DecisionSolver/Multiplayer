#include <conio.h>

using namespace std;
#include "MySQL/MySQL_Client.h"
#include "Client.hpp"
#include "Packet.hpp"

#include "Servers.hpp"

using namespace net;
using namespace network;

//#define NO_SERVERS

#ifndef NO_SERVERS
std::shared_ptr<Server> This_Server = std::make_shared<Server>();
std::shared_ptr<ServerFTP> This_Server_FTP = std::make_shared<ServerFTP>();
#endif

shared_ptr<mysql::Client> DB = make_shared<mysql::Client>();

vector<shared_ptr<Client>> Users;

#define IP "127.0.0.1"
#define PORT 25565

// New
#include "File System/Level/Levels.h"
#include "File System/File_system.h"

shared_ptr<File_system> FS;
#include "../File System/Project System/Project.h"

extern std::unique_ptr<ProjectFile> Project;

std::string Login, Pass;

//Test
nlohmann::json _Data;

bool UseRepeater = false, UseClear = false;
void ConnectFunc(string _Login, string _Pass)
{
	if (_Login.empty() || _Pass.empty()) return;

	Login = _Login;

	Users.push_back(make_shared<net::Client>("", ConnectionManager::TypeProtocol::TCP, 0));
#if defined(USE_SSL)
	Users.back()->Set_Cert_Chain("keys/rootca.crt");
	Users.back()->Set_Cert_RSA_Private("keys/user.key");
#endif
	if (Users.back()->Connect(IP, PORT))
	{
		Users.back()->StartSystem();

		// Create MySQL Packet That We're Connection To
		std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
		Answer->CreatePacket(network::Packet::Type::Login, true, { { "_0", _Login },
			{ "_1", Pass = md5_from_buffer(_Pass) } });

		Users.back()->Send(Answer);
	}
	else
	{
		system("cls");
#if __has_include("logger.h")
		Logger_Error_F("User: {}, didn't connect to {}", _Login, IP);
#endif
	}
}
void GetPacketFromThread(network::Packet packet, network::Packet::Type NeededPacket)
{
	/*
	if (packet && packet.getHeader().type == network::Packet::Type::Sync_File_Sync)
	{
		auto Obj = FS->GetFile(packet.getData()["_0"].get<json::string_t>());
		if (Obj && Obj->Size > 0)
		{
			std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
			packet->CreatePacket(network::Packet::Type::Sync_File_Sync, true,
				{
					{ "_0", "01.08.16.wav" },
					{ "_1", "AD0234829205B9033196BA818F7A872B" }
				});

			Users.back()->Send(packet);

			DebugBreak();
		}
		else
		{
			std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
			packet->CreatePacket(network::Packet::Type::Sync_File_Sync, true,
				{
					{ "_0", "01.08.16.wav" },
					{ "_1", "0" }
				});

			Users.back()->Send(packet);
		}

		return;
	}
	*/
	if (packet && packet.getHeader().type == network::Packet::Type::Sync_File && packet.getHeader().IsAnswer)
	{
		Users.back()->GetConnect()->getFtpClient()->Connect("127.0.0.1", Login, Pass, 21);
		auto Data = packet.getData();
		if (Data.find("_0") != Data.end() && (Data["_0"].is_boolean() && Data["_0"].get<json::boolean_t>()))
		{
			auto FName = Data["FName"].get<json::string_t>();
			std::string Path;
			if (Users.back()->GetConnect()->getFtpClient()->ReceiveFile(Data["_1"].get<json::string_t>(),
				Path = FS->getPathFromType(FS->GetTypeFileByExt(FName)) + FName))
			{
				Logger_Debug_F("File: {}. Successfully Downloaded And Placed In: {}", FName, Path);
			}
			else
			{
				Logger_Error_F("File: {}. Unsuccessful. See Logs! It Must Be In: {}", FName, Path);
			}
		}
		return;
	}
	if (packet && (packet.getHeader().type == NeededPacket) && packet.getHeader().IsAnswer)
	{
		json unparsed = packet.getData();
		size_t i = 0;
		for (size_t i = 0; i < unparsed["_0"].size(); i++)
		{
#if __has_include("logger.h")
			Logger_Info_F("\tID: {} - Name: {}\n", unparsed["_1"].at(i).front().get<json::number_integer_t>(),
				unparsed["_0"].at(i).front().get<json::string_t>());
#endif
			i++;
		}
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

#ifndef NO_SERVERS
	This_Server->Start();
	This_Server_FTP->Start();
#endif

	FS = make_shared<File_system>();

	std::thread([&]
	{
		while (true)
		{
			if (Project && (!Project->GetCurrentProject().empty() && Project->ThisLevel))
			{
				Project->ThisLevel->Update();
				auto Objs = Project->ThisLevel->getChild()->GetNodes();
				for (size_t i = 0; i < Objs.size(); i++)
				{
					auto Pos = Objs.at(i)->GM->GetPositionCord();
					float vPos[3] = { Pos.x, Pos.y, Pos.z };

					auto Rot = Objs.at(i)->GM->GetRotCord();
					float vRot[3] = { Rot.x, Rot.y, Rot.z };

					auto Scl = Objs.at(i)->GM->GetScaleCord();
					float vScl[3] = { Scl.x, Scl.y, Scl.z };

					//Logger_Info_F("Model indx: '%i', "\
					//	"Model ID: '%s', "\
					//	"Model R_ID: '%s', "\
					//	"Model Pos 'X=%f, Y=%f, Z=%f', "\
					//	"Model Scl 'X=%f, Y=%f, Z=%f', "\
					//	"Model Rot 'X=%f, Y=%f, Z=%f'",
					//	i, Objs.at(i)->ID.c_str(),
					//	Objs.at(i)->RenderName.c_str(),
					//	vPos[0], vPos[1], vPos[2],
					//	vScl[0], vScl[1], vScl[2],
					//	vRot[0], vRot[1], vRot[2]);

					Objs.front()->GM->SetPositionCoords({ random_floats(0, 3), random_floats(0, 2), random_floats(1,1) });
					this_thread::sleep_for(1s);
				}
			}
		}
	}).detach();

	auto fData = FS->LoadSettingsFile();
	Project->OpenOrCreateDB();

	int Choice = 0;
	while (true)
	{
#if __has_include("logger.h")
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
			string _Login, _Pass;
			system("cls");
#if __has_include("logger.h")
			Logger_Info("Enter Login Here: ");
			cin >> _Login;
#endif
#if __has_include("logger.h")
			Logger_Info("Enter Password Here: ");
			cin >> _Pass;
#endif

			ConnectFunc(_Login, _Pass);

			break;
		}
		case 2:
		{
			return 0;
		}
		default:
		{
			system("cls");
#if __has_include("logger.h")
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
				Sleep(100);
				network::Packet packet;

				if (Users.size() == 1)
				{
					if (!UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
						packet.clear();

						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
						GetPacketFromThread(packet, network::Packet::Type::Sync_File);
						packet.clear();
					}
					while (UseRepeater)
					{
						if (!Users.front()->GetConnect()) continue;
						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
						GetPacketFromThread(packet, network::Packet::Type::GetListUsersOnline);
						packet.clear();

						Users.front()->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
						GetPacketFromThread(packet, network::Packet::Type::Sync_File);
						packet.clear();
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

							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
							GetPacketFromThread(packet, network::Packet::Type::Sync_File);
							packet.clear();
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

							CurrentUser->GetConnect()->GetPacket(packet, network::Packet::Type::Sync_File);
							GetPacketFromThread(packet, network::Packet::Type::Sync_File);
							packet.clear();
						}
					}
				}

			}
		}).detach();
		while (true)
		{
			Sleep(1000);
#if __has_include("logger.h")
			Logger_Info("Choice The One:\n");
			Logger_Info("\t[0] - Get Ping (ECHO)\n");
			Logger_Info("\t[1] - Send Chat Message Packet\n");
			Logger_Info("\t[2] - Get File\n");
			Logger_Info("\t[3] - Send \"Sound Play\" Packet (By Default It's \"01.08.16.wav\")\n");

			Logger_Info("\t[4] - Get List Online Users\n");

			Logger_Info("\t[5] - Back To Previous Menu\n");

			Logger_Info_F("\t[6] - Use Repeater Any Packets ({} - is now)\n", UseRepeater ? "ON": "OFF");

			Logger_Info_F("\t[7] - Use Clear Screen After Command ({} - is now)\n", UseClear ? "ON": "OFF");

			Logger_Info(": ");
#endif
			cin >> Choice;
			switch (Choice)
			{
			case 0:
			{
				std::thread t = std::thread([&]
				{
					std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
					packet->CreatePacket(network::Packet::Type::Ping, false);

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
#if __has_include("logger.h")
				Logger_Info("Enter Message Here: ");
#endif
				cin >> Text;

				std::thread t = std::thread([&]
				{
					std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
					packet->CreatePacket(network::Packet::Type::Chat, false, { { "_0", Text + "\n" } });

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
				//std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				//packet->CreatePacket(network::Packet::Type::Sync_NewNode, true,
				//{
				//	{ "FName", "01.08.16.wav" },
				//	{ "id", "01.08.16" }
				//});
				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket(network::Packet::Type::Sync_File, false,
					{ { "FName", "01.08.16.wav" } });

				Users.back()->Send(packet);
				break;
			}
			case 3:
			{
				Text.clear();
				system("cls");
#if __has_include("logger.h")
				Logger_Info("Enter ID User Here (or '-1' To All Users That Are Online): ");
#endif
				cin >> Text;

				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket(network::Packet::Type::Chat, false, _Data);
				if (Users.front()->GetConnect())
					Users.front()->GetConnect()->Send(packet);
				
				break;
				// Doesn't Work Because DB Was Overwritten By Changing Design Of Networking
				if (Text == "-1")
				{
					auto AllUsersID = DB->SelectValues("Local", { "_N" }, { " WHERE _N = 1" });
					vector<std::shared_ptr<network::Packet>> packet;

					for (auto ID: AllUsersID)
					{
						packet.push_back(std::make_shared<network::Packet>());
						packet.back()->CreatePacket(network::Packet::Type::PlaySound, false,
							{
								{ "_0", ID.back().get<json::number_integer_t>() },
								{ "_1", 0.46f },
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
					std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
					//packet->CreatePacket(network::Packet::Type::PlaySound, false,
					//	{
					//		{ "_0", atoi(Text.c_str()) },
					//		{ "_1", 0.016f },
					//		{ "_2", "01.08.16.wav" }
					//	});
					packet->CreatePacket(network::Packet::Type::Chat, false, _Data);
					if (Users.front()->GetConnect())
						Users.front()->GetConnect()->Send(packet);
				}
				break;
			}
			case 4:
			{
				std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
				packet->CreatePacket(network::Packet::Type::GetListUsersOnline, false);

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

#ifndef NO_SERVERS
	This_Server->StopSystem();
	This_Server_FTP->StopSystem();
#endif

	return 0;
}
