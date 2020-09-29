#pragma once
#include <vector>
#include <string>
#include <list>
namespace swl
{
	class Client;
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
		void Worker(std::shared_ptr<swl::Client> this_client);
		void Save(std::string FileName, Packet Packet, swl::Client *this_client);
	};
}

