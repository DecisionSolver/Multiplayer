#pragma once
#include "pch.h"

namespace network
{
	class TCPClient;
	class Packet;
	class TCPSocket;
	class FileTransfer
	{
	private:
		std::list<Packet> packets;
		std::vector<char> dataFile;
	public:
		bool SeparateFileIntoPackets(std::string FileName, size_t HowManyParts = 1,
			int ID_Recipient = -1 /*it means everyone*/);
		void Worker(std::shared_ptr<network::TCPClient> this_client);
		void Save(std::string FileName, Packet Packet, network::TCPClient *this_client);
	};
}

