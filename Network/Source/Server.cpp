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
				if (connection == Next) continue;
				swl::Packet Answer = swl::Packet();
				json pack = Answer.CreateMessage();
				pack["data"]["body"].clear();
				pack["data"]["body"] = json::parse(packet.getData());
				Answer.FillIn(swl::Packet::Header(swl::Packet::Type::Chat), pack);
				Next->Send(Answer);
			}
		}
	}

	void Server::Start()
	{
		ConnectionManager::StartSystem([&](Connection::SharedPtr connection)
		{
			Server::OnPacketHandler(connection);
		});
		User->Connect("gb_z_rod2_rf", "696ea7b8ty", "mysql101.1gb.ru", "gb_z_rod2_rf");
	
		// Set All Users To Offline
		User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _2 = '1'" } });
	}

	void Server::Send(std::string Packet)
	{
		ConnectionManager::Send(Packet);
	}
}