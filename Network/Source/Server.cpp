#include "pch.h"
#include "Server.hpp"

namespace net
{
	void Server::OnPacketHandler(Connection::SharedPtr connection
	/* Came (One Connection) From Cycle m_connections */)
	{
		if (!connection) return;

		swl::Packet packet = swl::Packet();
		connection->GetPacket(packet, swl::Packet::Type::Chat);
		if (packet)
		{
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateMessage()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Chat), pack);
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateMessage()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Chat), pack);
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}

		connection->GetPacket(packet, swl::Packet::Type::GetListUsersOnline);
		if (packet)
		{
			json Arr;
			auto Obj = User->TrySelectValues("Local", { "*" }, { " WHERE _2 = 1" });
			for (size_t i = 0; i < Obj["_N"].size(); i++)
			{
				Arr["_1"].push_back(Obj["_N"].at(i).back().get<json::number_integer_t>());
				Arr["_0"].push_back(Obj["_0"].at(i).back().get<json::string_t>());
			}
			swl::Packet Answer = swl::Packet();
			json pack = json::parse(Answer.CreateAnswer()->getData());
			pack["data"]["body"]["_0"] = Arr["_0"]; // ID MySQL
			pack["data"]["body"]["_1"] = Arr["_1"]; // Login
			Answer.FillIn(swl::Packet::Header(swl::Packet::Type::GetListUsersOnline), pack);
			connection->Send(Answer);
			packet.clear();
		}

		connection->GetPacket(packet, swl::Packet::Type::PlaySound);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					if (Next.second->GetMetaDB_User() != unparse["_0"].get<int>()) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::PlaySound), pack);
					Next.second->Send(Answer);
					break;
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					if (Next.second->GetMetaDB_User() != unparse["_0"].get<int>()) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::PlaySound), pack);
					Next.second->Send(Answer);
					break;
				}
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_PosChanges);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_PosChanges), pack);
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_PosChanges), pack);
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_RotChanges);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_RotChanges), pack);
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_RotChanges), pack);
					Next.second->Send(Answer);
				}

			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_SclChanges);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
			{
				// Not To ME!!!
				if (connection == Next.second) continue;
				swl::Packet Answer = swl::Packet();
				json pack = json::parse(Answer.CreateAnswer()->getData());
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_SclChanges), pack);
				Next.second->Send(Answer);
			}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_SclChanges), pack);
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_NewNodeName);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_NewNodeName), pack);
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_NewNodeName), pack);
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_NewNode);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			if (_Proto == TypeProtocol::TCP)
			{
				for (auto &Next: m_connectionsTCP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_NewNode), pack);
					Next.second->Send(Answer);
				}
			}
			else if (_Proto == TypeProtocol::UDP)
			{
				for (auto &Next: m_connectionsUDP)
				{
					// Not To ME!!!
					if (connection == Next.second) continue;
					swl::Packet Answer = swl::Packet();
					json pack = json::parse(Answer.CreateAnswer()->getData());
					pack["data"]["body"].clear();
					pack["data"]["body"] = json::parse(packet.getData());
					Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Sync_NewNode), pack);
					Next.second->Send(Answer);
				}
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Get_MetaData_Project);
		if (packet)
		{

		}
		connection->GetPacket(packet, swl::Packet::Type::Get_MetaData_Project_Ex);
		if (packet)
		{

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
			WaitForMySQL.notify_all();
		else
		{
			printf("Something Is Went Wrong With Connection To MySQL Server!\n");
			WaitForMySQL.notify_all();
		}
		// Set All Users To Offline
		User->TryUpdateValues("Local", { "_2" }, { { "0" } }, { { " WHERE _2 = '1'" } });
	}

	void Server::Send(std::string Packet)
	{
		ConnectionManager::Send(Packet);
	}
}