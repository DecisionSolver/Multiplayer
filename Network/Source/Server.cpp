#include "pch.h"
#include "Server.hpp"

extern std::mutex m_connectionsMutex;

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
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Chat, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					pack["data"]["body"].clear();
					pack["data"]["body"] = packet.getData();
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					pack["data"]["body"].clear();
					pack["data"]["body"] = packet.getData();
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::GetListUsersOnline);
		if (packet)
		{
			json Arr;
			auto Obj = User->SelectValues("Local", { "*" }, { " WHERE _2 = 1" });
			for (size_t i = 0; i < Obj["_N"].size(); i++)
			{
				Arr["_1"].push_back(Obj["_N"].at(i).back().get<json::number_integer_t>());
				Arr["_0"].push_back(Obj["_0"].at(i).back().get<json::string_t>());
			}
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::GetListUsersOnline)->getData();
			pack["data"]["body"]["_0"] = Arr["_0"]; // ID MySQL
			pack["data"]["body"]["_1"] = Arr["_1"]; // Login
			Answer.FillIn(pack);
			connection->Send(Answer);
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::PlaySound);
		if (packet)
		{
			json unparse = packet.getData();
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::PlaySound, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
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
				for (auto &Next: m_connectionsUDP)
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
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::PlayVoice, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
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
				for (auto &Next: m_connectionsUDP)
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
		if (packet)
		{
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Sync_PosChanges, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_RotChanges);
		if (packet)
		{
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Sync_RotChanges, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}

			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_SclChanges);
		if (packet)
		{
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Sync_SclChanges, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_NewNodeName);
		if (packet)
		{
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Sync_NewNodeName, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Sync_NewNode);
		if (packet)
		{
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Sync_NewNode, true, packet.getData())->getData();
			Answer.FillIn(pack);
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, network::Packet::Type::Get_MetaData_Project);
		if (packet)
		{

		}

		connection->GetPacket(packet, network::Packet::Type::Get_MetaData_Project_Ex);
		if (packet)
		{

		}
		
		connection->GetPacket(packet, network::Packet::Type::Ping);
		if (packet)
		{
			network::Packet Answer = network::Packet();
			json pack = Answer.CreatePacket(network::Packet::Type::Ping)->getData();
			if (_Proto == TypeProtocol::TCP)
				connection->Send(Answer);
			else if (_Proto == TypeProtocol::UDP)
				connection->Send(Answer);
			packet.clear();
		}
	}

	void Server::Start()
	{
		ConnectionManager::StartSystem([&](Connection::SharedPtr connection)
		{
			Server::OnPacketHandler(connection);
		});
		if (User->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"188.210.240.246"
#endif
			, "gb_z_rod2_rf") == mysql::Client::Done)
		{
			WaitForMySQL.notify_all();
			// Set All Users To Offline
			User->UpdateValues("Local", { "_2" }, { { "0" } }, { { " WHERE _2 = '1'" } });
		}
		else
		{
#if defined(HAS_LOGGER)
		Logger_Critical("Something Is Went Wrong With Connection To MySQL Server!");
#endif
			WaitForMySQL.notify_all();
		}
	}

	void Server::Send(std::string Packet)
	{
		ConnectionManager::Send(Packet);
	}
}