#include "PackerLine.h"
std::pair<bool, std::pair<ConnectionManager::PoolWaiterPackets::ReturnType, std::pair<std::pair<int, bool>, std::pair<int, bool>>>>
PacketLine::Check(const std::chrono::seconds &TimeOut)
{
	std::unique_lock<std::mutex> ul(m_PacketWaiter);

	Cur = Last = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds Diff = (Last - Cur);

	// Create Loop For Non-block The Main Thread (to receive packets)
	while (Diff <= TimeOut)
	{
		Last = std::chrono::high_resolution_clock::now();
		Diff = (Last - Cur);

		// Wait For Acception Connection Packet From Server
		std::this_thread::sleep_for(100ms);

		if (NeedToBreak.load())
		{
			break;
		}
	}
	// If It Wasn't Triggered By Time-Out
	if (WasPacket.load() && !wasActive.load())
	{
		wasActive.store(true);
		return { true, CauseBreak };
	}
	else
	{
		return { false, CauseBreak };
	}
	return { false, CauseBreak };
}

void PacketLine::ProccessChain(std::shared_ptr<network::Packet> packet)
{
	std::unique_lock<std::mutex> ul(mProccessingChain);
	for (size_t i = 0; i < PacketChain.size(); i++)
	{
		// If It's Done Then Skip It
		if (PacketChain[i]->WasActive()) continue;

		// Checking For Status
		if (!PacketChain[i]->GetType().empty() && !PacketChain[i]->GetStatus().empty()
			&& PacketChain[i]->GetType().find(packet->getHeader().type) != PacketChain[i]->GetType().end())
		{
			auto obj = packet->getData();
			nlohmann::json::const_iterator res = obj.end();
			recursive_iterate(obj, [&](nlohmann::json::const_iterator it)
			{
				auto Obj = PacketChain[i]->GetStatus().find((*it));
				if ((*it).type() == nlohmann::json::value_t::number_integer &&
					Obj != PacketChain[i]->GetStatus().end())
				{
					res = it;
					return true;
				}
				return false;
			});
			if (res != obj.end())
			{
				if (PacketChain[i]->GetStatus().find((*res))->second)
				{
					for (size_t n = 0; n < PacketChain.size(); n++)
					{
						PacketChain[n]->SetWasPacket();

						PacketChain[n]->SetStatusCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first,
							PacketChain[i]->GetStatus().find((*res))->first);

						// Unblock "Check" Function
						PacketChain[n]->NotifyOne();
					}
				}
				else
				{
					PacketChain[i]->SetWasPacket();

					PacketChain[i]->SetStatusCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first,
						PacketChain[i]->GetStatus().find((*res))->first);

					// Unblock "Check" Function
					PacketChain[i]->NotifyOne();
				}
				break;
			}
		}
		else if (!PacketChain[i]->GetType().empty() &&
			PacketChain[i]->GetType().find(packet->getHeader().type) != PacketChain[i]->GetType().end())
		{
			if (PacketChain[i]->GetType().find(packet->getHeader().type)->second)
			{
				for (size_t n = 0; n < PacketChain.size(); n++)
				{
					PacketChain[n]->SetWasPacket();

					PacketChain[n]->SetTypeCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first);

					// Unblock "Check" Function
					PacketChain[n]->NotifyOne();
				}
			}
			else
			{
				PacketChain[i]->SetWasPacket();

				PacketChain[i]->SetTypeCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first);
				// Unblock "Check" Function
				PacketChain[i]->NotifyOne();
			}
			break;
		}
	}
}

void PacketLine::Disconnect()
{
	for (size_t i = 0; i < PacketChain.size(); i++)
	{
		PacketChain[i]->NotifyOne();
	}
}