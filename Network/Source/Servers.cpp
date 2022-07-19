#include "pch.h"
#include "Servers.hpp"

#include "../File System/Project System/Project.h"

extern std::mutex m_connectionsMutex;
extern std::unique_ptr<ProjectFile> Project;
#include "../File System/Level/Levels.h"
#include "../File System/File_system.h"
extern std::shared_ptr<File_system> FS;

//
std::pair<bool, std::vector<std::string>> _Projects;
std::shared_ptr<network::Packet> Get_AllProjects;
//

std::atomic_bool IsBlockByCommiting = false;
#include "server_impl.h"

namespace net
{
	void Server::OnPacketHandler(Connection::SharedPtr connection
	/* Came (One Connection) From Cycle m_connections */)
	{
		if (!connection) return;

		network::Packet packet = network::Packet();
		connection->GetPacket(packet, network::Packet::Type::Chat);
		if (packet)
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Chat, true, packet.getData());
#if !defined(USE_SSL)
			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next : m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
#endif 
			if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next : m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
		if (packet)
		{
			json Arr;
			auto Obj = MySQL_DB->SelectValues("", { "*" }, { " WHERE _2 = 1" });
			for (size_t i = 0; i < Obj["_N"].size(); i++)
			{
				Arr["_1"].push_back(Obj["_N"].at(i).back().get<json::number_integer_t>());
				Arr["_0"].push_back(Obj["_0"].at(i).back().get<json::string_t>());
			}
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::GetListUsersOnline, true,
				{ {"_0", Arr["_0"]}, {"_1", Arr["_1"]} });
			connection->Send(Answer);
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::PlaySound);
		if (packet)
		{
			json unparse = packet.getData();
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::PlaySound, true, packet.getData());
#if !defined(USE_SSL)
			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next : m_connectionsTCP)
				{
					Next.second->Send(Answer);
					if (unparse["_0"].get<int>() != -1)
						break;
					else if (Next.second->GetMetaDB_User() != unparse["_0"].get<int>())
						continue;
				}
			}
#endif 
			if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next : m_connectionsUDP)
				{
					Next.second->Send(Answer);
					if (unparse["_0"].get<int>() != -1)
						break;
					else if (Next.second->GetMetaDB_User() != unparse["_0"].get<int>())
						continue;
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_PosChanges);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_PosChanges, true, packet.getData());

			if (Project && (Project->ThisLevel && Project->ThisLevel->IsLoaded()))
			{
				json JSData = packet.getData();
				auto NeededNode = Project->ThisLevel->getChild()->getNodeByID(JSData["id"].get<json::string_t>());
				if (NeededNode && NeededNode->GM || !NeededNode->ID.empty())
				{
					auto END = JSData.end();
					vector<float> Float3;
					std::string Context;
					if (JSData.find("X") != END)
						Context += JSData["X"].get<json::string_t>();
					else
						Context += std::to_string(NeededNode->GM->GetPositionCord().x);
					if (JSData.find("Y") != END)
						Context += "," + JSData["Y"].get<json::string_t>();
					else
						Context += "," + std::to_string(NeededNode->GM->GetPositionCord().x);
					if (JSData.find("Z") != END)
						Context += "," + JSData["Z"].get<json::string_t>();
					else
						Context += "," + std::to_string(NeededNode->GM->GetPositionCord().x);

					getFloat3Text(Context, ",", Float3);
					NeededNode->GM->SetPositionCoords(Vector3(Float3.data()));

					NeededNode->IsItChanged = true;
					NeededNode->SaveInfo->Pos = true;
					NeededNode->SaveInfo->T = NeededNode->GM->GetType();
				}
			}

			SendPacket(true, connection, Answer);

			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_RotChanges);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_RotChanges, true, packet.getData());

			if (Project && (Project->ThisLevel && Project->ThisLevel->IsLoaded()))
			{
				json JSData = packet.getData();
				auto NeededNode = Project->ThisLevel->getChild()->getNodeByID(JSData["id"].get<json::string_t>());
				if (NeededNode && NeededNode->GM || !NeededNode->ID.empty())
				{
					auto END = JSData.end();
					vector<float> Float3;
					std::string Context;
					if (JSData.find("X") != END)
						Context += JSData["X"].get<json::string_t>();
					else
						Context += std::to_string(NeededNode->GM->GetRotCord().x);
					if (JSData.find("Y") != END)
						Context += "," + JSData["Y"].get<json::string_t>();
					else
						Context += "," + std::to_string(NeededNode->GM->GetRotCord().x);
					if (JSData.find("Z") != END)
						Context += "," + JSData["Z"].get<json::string_t>();
					else
						Context += "," + std::to_string(NeededNode->GM->GetRotCord().x);

					getFloat3Text(Context, ",", Float3);
					NeededNode->GM->SetRotationCoords(Vector3(Float3.data()));

					NeededNode->IsItChanged = true;
					NeededNode->SaveInfo->Rot = true;
					NeededNode->SaveInfo->T = NeededNode->GM->GetType();
				}
			}

			SendPacket(true, connection, Answer);

			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_SclChanges);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_SclChanges, true, packet.getData());

			if (Project && (Project->ThisLevel && Project->ThisLevel->IsLoaded()))
			{
				json JSData = packet.getData();
				auto NeededNode = Project->ThisLevel->getChild()->getNodeByID(JSData["id"].get<json::string_t>());
				if (NeededNode && NeededNode->GM || !NeededNode->ID.empty())
				{
					auto END = JSData.end();
					vector<float> Float3;
					std::string Context;
					if (JSData.find("X") != END)
						Context += JSData["X"].get<json::string_t>();
					else
						Context += std::to_string(NeededNode->GM->GetScaleCord().x);
					if (JSData.find("Y") != END)
						Context += "," + JSData["Y"].get<json::string_t>();
					else
						Context += "," + std::to_string(NeededNode->GM->GetScaleCord().x);
					if (JSData.find("Z") != END)
						Context += "," + JSData["Z"].get<json::string_t>();
					else
						Context += "," + std::to_string(NeededNode->GM->GetScaleCord().x);

					getFloat3Text(Context, ",", Float3);
					NeededNode->GM->SetScaleCoords(Vector3(Float3.data()));

					NeededNode->IsItChanged = true;
					NeededNode->SaveInfo->Scale = true;
					NeededNode->SaveInfo->T = NeededNode->GM->GetType();
				}
			}

			SendPacket(true, connection, Answer);

			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_NewNodeName);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_NewNodeName, true, packet.getData());

			if (Project && (Project->ThisLevel && Project->ThisLevel->IsLoaded()))
			{
				json JSData = packet.getData();
				auto NeededNode = Project->ThisLevel->getChild()->getNodeByID(JSData["id"].get<json::string_t>());
				if (NeededNode && !NeededNode->ID.empty())
				{
					NeededNode->RenderName = JSData.find("NodeName") != JSData.end()
						? JSData["NodeName"].get<json::string_t>() : NeededNode->RenderName;

					// In This "IsItChanged" Means That It Changes Only Name Of Node
					NeededNode->IsItChanged = true;
					Project->ThisLevel->SetNotSaved(true);
				}
			}

			SendPacket(true, connection, Answer);

			packet.clear();
		}

		// Came If User Needs Some File (We Need To Indetify Location Of The File)
		connection->GetPacket(packet, network::Packet::Type::Sync_File);
		if (packet && !IsBlockByCommiting.load())
		{
			// We Have File That Need To Send Back
			json JSData = packet.getData();
			if (JSData.find("FName") != JSData.end() && (!JSData["FName"].empty()))
			{
				std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
				
				// Try To Find This File In Resources Of Engine
				auto Obj = FS->Find(JSData["FName"].get<json::string_t>(), false);
				// If File Is Exist
				if (Obj && Obj->Size > 0)
				{
					path FilePath = FS->GetFTPPath() + "Users/" + std::to_string(connection->GetMetaDB_User()) + "/" +
						Obj->FName.string();

					bool IfCopyDone = false;

					try
					{
						IfCopyDone = boost::filesystem::copy_file(Obj->Path, FilePath,
							boost::filesystem::copy_option::overwrite_if_exists);
					}
					catch (boost::filesystem::filesystem_error const &e)
					{
#if __has_include("logger.h")
						Logger_Error_F("{}", e.what());
#endif
					}

					if (IfCopyDone)
					{
						// If File Not Found
						Answer->CreatePacket(network::Packet::Type::Sync_File, true,
						{
							{ "_0", FilePath.string() }
						});

						connection->Send(Answer);

						Answer->clear();
					}
				}
				else
				{
					// Or Find It In FTP Resources Of User's Files
					// Note: We Need To Send ONLY Relative Path To FTP User's Folder
					//		And Then FTP Check User Wright By Itself When Will FTP Client Try File
				
					auto AllUsersFilesFTP = FS->getFilesInFolder(FS->GetFTPPath() + "Users/", true, true);
					for (const auto &File: AllUsersFilesFTP)
					{
						auto LowercaseFname = File.filename().string();
						boost::to_lower(LowercaseFname);
						auto LowercaseAnotherFname = JSData["FName"].get<json::string_t>();
						boost::to_lower(LowercaseAnotherFname);
						if (LowercaseFname == LowercaseAnotherFname)
						{
							std::string ret = boost::filesystem::relative(File, FS->GetFTPPath()).string();
							// If File Not Found
							Answer->CreatePacket(network::Packet::Type::Sync_File, true,
							{
								{ "_0", ret },
							});

							connection->Send(Answer);

							Answer->clear();
							break;
						}
					}
				}
			}
			
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_NewNode);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_NewNode, true, packet.getData());
			
			bool Need2Send = true;

			if (Project && (Project->ThisLevel && Project->ThisLevel->IsLoaded()))
			{
				json JSData = packet.getData();
				if (JSData.find("FName") != JSData.end() && (!JSData["FName"].empty()))
				{
					auto PacketData = packet.getData();
					// Check File In Resources Of Engine
					path FilePath = FS->GetFTPPath() + "Users/" +
						std::to_string(connection->GetMetaDB_User()) + "/" + PacketData["FName"].get<json::string_t>();

					if (!exists(FilePath))
					{
						Answer->CreatePacket(network::Packet::Type::Sync_File, true,
							{ "Info", "File Isn't Exist On a Server Yet!" });

						connection->Send(Answer);

						Answer->clear();

						Need2Send = false;
					}
					else
					{
						auto Obj = FS->Find(PacketData["FName"].get<json::string_t>(), false);
						// If File Is NOT Exist
						if (Obj && Obj->Size == 0)
						{
							// But File Has Uploaded By User In FTP
							if (exists(FilePath))
							{
								// Need To Use This File In Resources
								FS->OnlyAddFile(FilePath);
							}
						}
						else
						{
							// If File Is Exists In Resource Of Engine

							// Need To Compare These MD5 Datas
							// If File Were Changed
							if (PacketData["_1"] != Obj->Hash /*HashFile*/)
							{
								try
								{
									boost::filesystem::copy_file(FilePath, Obj->Path,
										boost::filesystem::copy_option::overwrite_if_exists);
								}
								catch (boost::filesystem::filesystem_error const &e)
								{
#if __has_include("logger.h")
									Logger_Error_F("{}", e.what());
#endif
								}

								Obj->Hash = md5_from_file(Obj->Path.string());
								Obj->Size = (size_t)file_size(Obj->Path);
							}

							auto ND = Project->ThisLevel->Add(Obj->Path.string());
							if (ND && !ND->ID.empty())
								const_cast<std::string &>(ND->ID) = JSData["id"];

							// If File Not Found
							Answer->CreatePacket(network::Packet::Type::Sync_File, true,
							{
								{ "FName", JSData["FName"] },
								{ "id", JSData["id"] },
								{ "_0", true },
								{ "_1", FilePath.string() },
							});
						}
					}
				}
			}
			else
			{
				Answer->CreatePacket(network::Packet::Type::Sync_File, true,
					{ "Info", "The Project Doesn't Open On This Server!" });

				connection->Send(Answer);

				Answer->clear();

				Need2Send = false;
			}

			if (Need2Send)
				SendPacket(true, connection, Answer);

			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Get_MetaData_Project);
		if (packet && !IsBlockByCommiting.load())
		{
			auto JData = packet.getData();
			auto end = JData.end();
			if (Project->Open(JData["_0"].get<json::string_t>(),
				(JData.find("_1") != end ? JData["_1"].get<json::string_t>(): ""),
				connection, network::Packet::Type::Get_MetaData_Project) == E_FAIL)
			{
#if __has_include("logger.h")
				Logger_Error_F("Can't Open Project: {}!", JData["_0"].get<json::string_t>());
#endif
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Get_MetaData_Project_Ex);
		if (packet && !IsBlockByCommiting.load())
		{
			auto JData = packet.getData();
			auto end = JData.end();
			if (Project->Open(JData["_0"].get<json::string_t>(),
				(JData.find("_1") != end ? JData["_1"].get<json::string_t>(): ""),
				connection, network::Packet::Type::Get_MetaData_Project_Ex) == E_FAIL)
			{
#if __has_include("logger.h")
				Logger_Error_F("Can't Open Project: {}!", JData["_0"].get<json::string_t>());
#endif
			}

			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Ping);
		if (packet)
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Ping);
			connection->Send(Answer);
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Get_AllProjects);
		if (packet)
		{
			if (Project)
			{
				if (_Projects.second.empty() && Project->DataBase)
				{
					ProjectFile::OpenOrCreateDB();
					_Projects = Project->DataBase->GetListTablesDatabase();
				}

				std::shared_ptr<network::Packet> Nothing;

				if (!Get_AllProjects)
				{
					if (_Projects.first)
					{
						Get_AllProjects = std::make_shared<network::Packet>();
						json DataToSend;
						for (size_t i = 0; i < _Projects.second.size(); i++)
						{
							auto Data = Project->DataBase->SelectValues(_Projects.second.at(i),
								{ "Date Create", "Hash Commit" });

							std::string Date = (!Data.is_discarded() &&
								(!Data.empty() && Data.find("Date Create") != Data.end()
									&& (!Data["Date Create"].at(1).is_discarded()))) ?
								Data["Date Create"].at(1).get<json::string_t>(): "NONE";

							DataToSend["_0"].push_back({ { "_0", _Projects.second.at(i) },
								{ "_1", Date }, { "_2", Data["Hash Commit"] } });
						}
						Get_AllProjects->CreatePacket(network::Packet::Type::Get_AllProjects, true, DataToSend);
					}
					else
					{
						Nothing = std::make_shared<network::Packet>();
						Nothing->CreatePacket(network::Packet::Type::Get_AllProjects, true, { { "Nothing" } });
					}
				}
				// If Has Something
				if (Get_AllProjects)
					connection->Send(Get_AllProjects);
				// Send That It Has Nothin'
				else
				{
					if (!Nothing)
					{
						Nothing = std::make_shared<network::Packet>();
						Nothing->CreatePacket(network::Packet::Type::Get_AllProjects, true, { { "Nothing" } });
					}

					connection->Send(Nothing);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Make_Commit);
		if (packet)
		{
			if (Project && Project->ThisLevel && Project->ThisLevel->IsLoaded())
			{
				auto Obj = connection->m_owner->MySQL_DB->SelectValues("", { "_3", "_0" }, { "WHERE _N = '" +
						std::to_string(connection->GetMetaDB_User()) + "';" });

				if (!Obj.empty() && Obj["_0"].front() == 1)
				{
					IsBlockByCommiting.store(true);

					std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
					Answer->CreatePacket(network::Packet::Type::IsCommiting, true);

					SendPacket(false, connection, Answer);

					if (!Project->ThisLevel->Commit(Obj["_1"].front(), packet.getData()["_0"]))
					{
						Answer->clear();
						Answer = nullptr;
						Answer = std::make_shared<network::Packet>();
						Answer->CreatePacket(network::Packet::Type::IsCommitingFailed, true,
						{
							{
								"Err",
								"Can't Make Commit In Project: " + Project->GetCurrentProject() + "!",
							}
						});

#if __has_include("logger.h")
						Logger_Error_F("Can't Make Commit In Project: {}!", Project->GetCurrentProject());
#endif
					}
					else
					{
						Answer->clear();
						Answer = nullptr;
						Answer = std::make_shared<network::Packet>();
						Answer->CreatePacket(network::Packet::Type::IsCommitingDone, true);
					}

					SendPacket(false, connection, Answer);

					IsBlockByCommiting.store(false);
				}
				else
				{
					std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
					Answer->CreatePacket(network::Packet::Type::IsCommitingFailed, true,
					{
						{
							"Err",
							"User Doesn't Have Permissions To Make Commits To The Server!",
						}
					});

					connection->Send(Answer);
				}
			}
			packet.clear();
		}
	}

	void Server::Start()
	{
		ConnectionManager::StartSystem([&](Connection::SharedPtr connection)
		{
			Server::OnPacketHandler(connection);
		});
		WaitForMySQL.notify_all();
		// Set All Users To Offline
		MySQL_DB->UpdateValues("", std::vector<std::string>{ { "_2" } }, {{"0"}}, {{" WHERE _2 = '1'"}});
	}

	void Server::Send(const std::string &Packet)
	{
		ConnectionManager::Send(Packet);
	}
	void Server::Send(const std::shared_ptr<network::Packet> &Packet)
	{
		ConnectionManager::Send(Packet);
	}
	void ServerFTP::Start()
	{
		ConnectionManager::StartSystem();
	}
}