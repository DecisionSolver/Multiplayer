#include "pch.h"
#include "Server.hpp"

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
			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
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
			auto Obj = MySQL_DB->SelectValues("Local", { "*" }, { " WHERE _2 = 1" });
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
			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					Next.second->Send(Answer);
					if (unparse["_0"].get<int>() != -1)
						break;
					else if (Next.second->GetMetaDB_User() != unparse["_0"].get<int>())
						continue;
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
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

		connection->GetPacket(packet, network::Packet::Type::PlayVoice);
		if (packet)
		{
			json unparse = packet.getData();
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::PlayVoice, true, packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					Next.second->Send(Answer);
					if (unparse["_0"].get<int>() != -1)
						break;
					else if (Next.second->GetMetaDB_User() != unparse["_0"].get<int>())
						continue;
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
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
					Vector3 NewPos = Vector3(
						JSData.find("X") != END ? std::stof(JSData["X"].get<json::string_t>().c_str())
						: NeededNode->GM->GetPositionCord().x,
						JSData.find("Y") != END ? std::stof(JSData["Y"].get<json::string_t>().c_str())
						: NeededNode->GM->GetPositionCord().y,
						JSData.find("Z") != END ? std::stof(JSData["Z"].get<json::string_t>().c_str())
						: NeededNode->GM->GetPositionCord().z);
					NeededNode->GM->SetPositionCoords(NewPos);

					NeededNode->IsItChanged = true;
					NeededNode->SaveInfo->Pos = true;
					NeededNode->SaveInfo->T = NeededNode->GM->GetType();
				}
			}

			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
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
					Vector3 NewRot = Vector3(
						JSData.find("X") != END ? std::stof(JSData["X"].get<json::string_t>().c_str())
						: NeededNode->GM->GetRotCord().x,
						JSData.find("Y") != END ? std::stof(JSData["Y"].get<json::string_t>().c_str())
						: NeededNode->GM->GetRotCord().y,
						JSData.find("Z") != END ? std::stof(JSData["Z"].get<json::string_t>().c_str())
						: NeededNode->GM->GetRotCord().z);
					NeededNode->GM->SetRotationCoords(NewRot);
				
					NeededNode->IsItChanged = true;
					NeededNode->SaveInfo->Rot = true;
					NeededNode->SaveInfo->T = NeededNode->GM->GetType();
				}
			}

			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}

			}
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
					Vector3 NewScale = Vector3(
						JSData.find("X") != END ? std::stof(JSData["X"].get<json::string_t>().c_str())
						: NeededNode->GM->GetScaleCord().x,
						JSData.find("Y") != END ? std::stof(JSData["Y"].get<json::string_t>().c_str())
							: NeededNode->GM->GetScaleCord().y,
						JSData.find("Z") != END ? std::stof(JSData["Z"].get<json::string_t>().c_str())
							: NeededNode->GM->GetScaleCord().z);
					NeededNode->GM->SetScaleCoords(NewScale);
				
					NeededNode->IsItChanged = true;
					NeededNode->SaveInfo->Scale = true;
					NeededNode->SaveInfo->T = NeededNode->GM->GetType();
				}
			}

			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
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

			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_NewNode);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_NewNode, true, packet.getData());

			if (Project && (Project->ThisLevel && Project->ThisLevel->IsLoaded()))
			{
				json JSData = packet.getData();
				if (JSData.find("FName") != JSData.end() && (!JSData["FName"].empty()))
				{
					auto FileObj = FS->GetFile(JSData["FName"].get<json::string_t>());
					if (FileObj && (FileObj->Size > 0 && !FileObj->Path.empty()))
					{
						auto Obj = Project->ThisLevel->Add(FileObj->Path.string());
						if (Obj && !Obj->ID.empty())
							Obj->ID = JSData["id"];
					}
				}
			}

			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
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
				Logger_Error_F("Can't Open Project: %s!", JData["_0"].get<json::string_t>().c_str());
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
				Logger_Error_F("Can't Open Project: %s!", JData["_0"].get<json::string_t>().c_str());
#endif
			}

			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_File_Sync);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_File, true, packet.getData());

			bool IfNeed2Send = false;
			if (packet.getHeader().IsAnswer)
			{
				// Need To Add FTP Main Client For TCP Server To Check Hash Of File

				// If File Were Changed Or Not Exist
				if (packet.getData()["_1"] != "AD0234829205B9033196BA818F7A872B" /*HashFile*/)
				{
					IfNeed2Send = true;
				}
			}

			if (IfNeed2Send)
			{
				if (_Proto == TypeProtocol::TCP)
				{
					for (const auto &Next: m_connectionsTCP)
					{
						// To ME!!!
						if (connection != Next.second) continue;
						Next.second->Send(Answer);
					}
				}
				else if (_Proto == TypeProtocol::UDP)
				{
					for (const auto &Next: m_connectionsUDP)
					{
						// To ME!!!
						if (connection != Next.second) continue;
						Next.second->Send(Answer);
					}
				}
			}
			packet.clear();
		}
		connection->GetPacket(packet, network::Packet::Type::Sync_File);
		if (packet && !IsBlockByCommiting.load())
		{
			std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
			Answer->CreatePacket(network::Packet::Type::Sync_File_Sync, false, packet.getData());

			if (_Proto == TypeProtocol::TCP)
			{
				for (const auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (const auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
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
				auto Obj = connection->m_owner->MySQL_DB->SelectValues("Local", { "_3", "_0" }, { "WHERE _N = '" +
						std::to_string(connection->GetMetaDB_User()) + "';" });

				if (!Obj.empty() && Obj["_0"].front() == 1)
				{
					IsBlockByCommiting.store(true);

					std::shared_ptr<network::Packet> Answer = std::make_shared<network::Packet>();
					Answer->CreatePacket(network::Packet::Type::IsCommiting, true);
					if (_Proto == TypeProtocol::TCP)
					{
						for (const auto &Next: m_connectionsTCP)
						{
							Next.second->Send(Answer);
						}
					}
					else if (_Proto == TypeProtocol::UDP)
					{
						for (const auto &Next: m_connectionsUDP)
						{
							Next.second->Send(Answer);
						}
					}

					if (!Project->ThisLevel->Commit(Obj["_1"].front(), packet.getData()["_0"]))
					{
						Answer->clear();
						Answer = nullptr;
						Answer = std::make_shared<network::Packet>();
						Answer->CreatePacket(network::Packet::Type::IsCommitingFailed, true, { { "Err",
							"Can't Make Commit In Project: " + Project->GetCurrentProject() + "!" } });

#if __has_include("logger.h")
						Logger_Error_F("Can't Make Commit In Project: %s!", Project->GetCurrentProject().c_str());
#endif
					}
					else
					{
						ToDo("Move It Away");
						Project->ThisLevel->SetNotSaved(false);

						Answer->clear();
						Answer = nullptr;
						Answer = std::make_shared<network::Packet>();
						Answer->CreatePacket(network::Packet::Type::IsCommitingDone, true);
					}

					if (_Proto == TypeProtocol::TCP)
					{
						for (const auto &Next: m_connectionsTCP)
						{
							Next.second->Send(Answer);
						}
					}
					else if (_Proto == TypeProtocol::UDP)
					{
						for (const auto &Next: m_connectionsUDP)
						{
							Next.second->Send(Answer);
						}
					}

					IsBlockByCommiting.store(false);
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
		if (MySQL_DB->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"188.210.240.246"
#endif
			, "gb_z_rod2_rf") == mysql::Client::Done)
		{
			WaitForMySQL.notify_all();
			// Set All Users To Offline
			MySQL_DB->UpdateValues("Local", { "_2" }, { { "0" } }, { { " WHERE _2 = '1'" } });
		}
		else
		{
#if __has_include("logger.h")
		Logger_Critical("Something Is Went Wrong With Connection To MySQL Server!");
#endif
			WaitForMySQL.notify_all();
		}
	}

	void Server::Send(const std::string &Packet)
	{
		ConnectionManager::Send(Packet);
	}
	void Server::Send(const std::shared_ptr<network::Packet> &Packet)
	{
		ConnectionManager::Send(Packet);
	}
}