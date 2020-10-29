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
		User->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"192.168.1.2"
#endif
			, "gb_z_rod2_rf");
	
		// Set All Users To Offline
		User->TryInsertValues("Local", { "_2" }, { { "0" } }, { { " WHERE _2 = '1'" } });
	}

	void Server::Send(std::string Packet)
	{
		ConnectionManager::Send(Packet);
	}
}