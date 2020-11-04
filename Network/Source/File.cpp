#include "pch.h"
#include "File.hpp"
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <Client.hpp>
#include <Server.hpp>
namespace swl
{
	bool FileTransfer::SeparateFileIntoPackets(std::string FileName, size_t HowManyParts, int ID_Recipient)
	{
		return false;
		/*std::fstream fileIn(FileName, std::ios::in | std::ios::binary);
		if (!fileIn)
		{
			OutputDebugStringA("FileTransfer::SeparateFileIntoPackets: !fileIn.is_open()");
			throw std::exception("FileTransfer::SeparateFileIntoPackets: !fileIn.is_open()");
			return false;
		}
		size_t length = 0, NewLenght = 0;
		fileIn.unsetf(std::ios::skipws);
		fileIn.seekg(0, std::ios::end);
		length = static_cast<int>(fileIn.tellg());
		fileIn.seekg(0, std::ios::beg);

		swl::Packet newPacket = swl::Packet();
		std::shared_ptr<std::vector<char>>buffer = std::make_shared<std::vector<char>>(1024 * 1024 * 1);
		try
		{
			if (HowManyParts > 1)
			{
				size_t i = 0;
				std::stringstream strStream;
				NewLenght = (length / HowManyParts);

				//std::fstream in(fileName + ".exe", std::ios::out | std::ios::binary);		

				fileIn.read(buffer->data(), static_cast<std::streamsize>(buffer->size()));
				auto bytes_read = fileIn.gcount();
				buffer->resize(static_cast<size_t>(bytes_read));

				unsigned int wasMoved = NewLenght;
				while (!buffer->empty())
				{
					std::vector<char> NewBuff(NewLenght);

					if (buffer->size() < NewLenght)
						wasMoved = buffer->size();
					std::copy(buffer->begin(), buffer->begin() + wasMoved, NewBuff.begin());
					// This Was Used Then Clear That Useless Area Of Memory
					buffer->erase(buffer->begin(), buffer->begin() + wasMoved);
				
					newPacket.clear();
					// Read By Parts And Add To New Packet

					json data = newPacket.CreateAnswer();
					data["header"]["_t"] = swl::Packet::Type::File;
					data["data"]["body"]["_0"] = NewBuff.data();

					data["data"]["_i"] = i + 1 == HowManyParts ? 255 : i;
					i++;
					if (ID_Recipient > -1)
						data["header"]["_R"] = ID_Recipient;
					else
						data["header"]["_R"] = "ALL";

					newPacket.FillIn(data);
					packets.insert(packets.begin(), newPacket);

					//	in.write(NewBuff.data(), static_cast<std::streamsize>(wasMoved));
				}
				fileIn.close();
			}
			else
			{
				newPacket.clear();
				// Read By Parts And Add To New Packet
				fileIn.read(buffer->data(), static_cast<std::streamsize>(buffer->size()));
				auto bytes_read = fileIn.gcount();
				buffer->resize(static_cast<size_t>(bytes_read));

				json data = newPacket.CreateAnswer();
				data["header"]["_t"] = swl::Packet::Type::File;
				data["data"]["body"]["_0"] = buffer->data();
				data["data"]["_i"] = 255;
				if (ID_Recipient > -1)
					data["header"]["_R"] = ID_Recipient;
				else
					data["header"]["_R"] = "ALL";

				newPacket.FillIn(data);
				packets.push_back(newPacket);
			}
		}
		catch (const std::exception&err)
		{
			OutputDebugStringA(("\n" + std::string(err.what()) + "\n").c_str());
			fileIn.close();
			return false;
		}

		fileIn.close();
		return true;
		*/
	}

	// Use It Only In Client->SEND!
	void FileTransfer::Worker(std::shared_ptr<swl::TCPClient> this_client)
	{
		/*if (!this_client)
			return;
		std::thread([&](std::shared_ptr<swl::TCPClient> this_client)
		{
			uint32_t ID = 0;

			std::list<swl::Packet>::iterator Obj = packets.begin();
			int PacketID = 0;
			do
			{
				if (packets.empty() || !*Obj || !Obj._Ptr || !Obj->operator bool()) return;
				this_client->send(*Obj);
				// Do Send!

				swl::Packet Packet = swl::Packet();
				if (this_client)
				{
					while (true)
					{
						Packet.receive(this_client->getSocket()->getSocket());
						if (Packet && Packet.getHeader().type & (swl::Packet::Type::Answer << swl::Packet::Type::File))
						{
							if (json::parse(Packet.getData())["data"]["body"]["_1"].get<std::string>() == "OK")
							{
								PacketID++;
								packets.pop_front();
								if (!packets.empty())
									std::advance(Obj, PacketID);
								break;
							}
							if (json::parse(Packet.getData())["data"]["body"]["_1"].get<std::string>() == "ERR")
								break; // TRY AGAIN!!!
						}
					}
				}
			} while (this_client->isConnected() && packets.size() != 0);
		}, this_client).detach();
		*/
	}

	// Use It Only In Client->RECV!
	void FileTransfer::Save(std::string FileName, swl::Packet Packet, swl::TCPClient *this_client)
	{
		/*if (Packet && Packet.getHeader().type & (swl::Packet::Type::Answer << swl::Packet::Type::File))
		{
			json dataJSON = json::parse(Packet.getData());
			if (dataJSON["data"]["_i"] <= 254)
			{
				// Read Data Packet To All Data And After That Save To File!
				
				//dataJSON["data"]["body"]["_0"].from_msgpack(dataJSON["data"]["body"]["_0"]);
				std::string data = dataJSON["data"]["body"].value("_0", "");

				std::copy(data.begin(), data.end(), std::back_inserter(dataFile));
				//dataFile.push_back('\0');

				Packet.clear();
				dataJSON = Packet.CreateAnswer();
				dataJSON["header"]["_t"] = swl::Packet::Type::File;
				dataJSON["data"]["body"]["_1"] = "OK";
				uint8_t newType = dataJSON["header"]["_t"].get<uint8_t>();
				newType |= (swl::Packet::Type::Answer << swl::Packet::Type::File);
				dataJSON["header"]["_t"] = newType;

				Packet.FillIn(dataJSON);

				// Send Answer That "OK"
				this_client->send(Packet);
			}
			else if (!dataJSON.empty() && dataJSON["data"]["_i"] == 255)
			{
				if (dataFile.empty())
				{
					//dataJSON["data"]["body"]["_0"] = json::from_msgpack(dataJSON["data"]["body"]["_0"]);
					std::string data = dataJSON["data"]["body"].value("_0", "");
					std::copy(data.begin(), data.end(), std::back_inserter(dataFile));
					//dataFile.push_back('\0');
				}

				std::ofstream fileOut;
				fileOut.open(FileName, std::ios::binary);
				fileOut.write(dataFile.data(), dataFile.size());
				fileOut.close();
			}
		}
		*/
	}
}