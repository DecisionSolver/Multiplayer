#include "pch.h"
#include "Server.hpp"

namespace net
{
	void Server::OnPacketHandler(Connection::SharedPtr connection /* Came (One Connection) From Cycle m_connections */)
	{
		if (!connection) return;

		swl::Packet packet = swl::Packet();
		connection->GetPacket(packet, swl::Packet::Type::Chat);
		if (packet)
		{
			for (auto &Next: m_connections)
			{
				// Even Not To ME!!!
				if (connection == Next) continue;
				swl::Packet Answer = swl::Packet();
				json pack = Answer.CreateMessage();
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Chat), pack);
				Next->Send(Answer);
			}
			packet.clear();
		}

		connection->GetPacket(packet, swl::Packet::Type::GetListUsersOnline);
		if (packet)
		{
			json Arr;
			auto Obj = User->TrySelectValues("Local", { "*" }, { " WHERE _2 = 1" });
			for (const auto &Next: Obj)
			{
				if (!Next.second.empty())
				{
					Arr["_1"].push_back((int)Next.second["_N"].get<json::value_t>());
					Arr["_0"].push_back(Next.second["_0"].get<json::string_t>());
				}
			}
			swl::Packet Answer = swl::Packet();
			json pack = Answer.CreateMessage();

			pack["data"]["body"]["_0"] = Arr["_0"]; // ID MySQL
			pack["data"]["body"]["_1"] = Arr["_1"]; // Login
			Answer.FillIn(swl::Packet::Header(swl::Packet::Type(swl::Packet::Type::Answer <<
				swl::Packet::Type::GetListUsersOnline)), pack);
			connection->Send(Answer);
			packet.clear();
		}

		connection->GetPacket(packet, swl::Packet::Type::PlaySound);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			for (auto &Next: m_connections)
			{
				if (Next->GetMetaDB_User() != unparse["_0"].get<int>()) continue;
				swl::Packet Answer = swl::Packet();
				json pack = Answer.CreateMessage();
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type(swl::Packet::Type::Answer <<
					swl::Packet::Type::PlaySound)), pack);
				Next->Send(Answer);
				break;
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_PosChanges);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			for (auto &Next: m_connections)
			{
				// Even Not To ME!!!
				if (connection == Next) continue;
				swl::Packet Answer = swl::Packet();
				json pack = Answer.CreateMessage();
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type(swl::Packet::Type::Answer <<
					swl::Packet::Type::Sync_PosChanges)), pack);
				Next->Send(Answer);
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_RotChanges);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			for (auto &Next : m_connections)
			{
				// Even Not To ME!!!
				if (connection == Next) continue;
				swl::Packet Answer = swl::Packet();
				json pack = Answer.CreateMessage();
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type(swl::Packet::Type::Answer <<
					swl::Packet::Type::Sync_RotChanges)), pack);
				Next->Send(Answer);
			}
			packet.clear();
		}
		connection->GetPacket(packet, swl::Packet::Type::Sync_SclChanges);
		if (packet)
		{
			json unparse = json::parse(packet.getData());
			for (auto &Next : m_connections)
			{
				// Even Not To ME!!!
				if (connection == Next) continue;
				swl::Packet Answer = swl::Packet();
				json pack = Answer.CreateMessage();
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type(swl::Packet::Type::Answer <<
					swl::Packet::Type::Sync_SclChanges)), pack);
				Next->Send(Answer);
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
		User->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"192.168.1.2"
#endif
			, "gb_z_rod2_rf");
		//User->Connect("gb_z_rod2_rf", "696ea7b8ty", "mysql101.1gb.ru", "gb_z_rod2_rf");

		// Set All Users To Offline
		User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _2 = '1'" } });
	}

	void Server::Send(std::string Packet)
	{
		ConnectionManager::Send(Packet);
	}
}