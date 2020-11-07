#include "pch.h"
#include "Packet.hpp"
#include <fstream>
#include <iostream>
#include "LZ4/lz4.h"

namespace swl
{
	void Packet::clear()
	{
		if (!data.empty())
			data.clear();
		_H.OrigSize = 0;
		_H.Settings = 0;
		_H.type = Type::Chat;
	}
	size_t Packet::getSize() const
	{
		return data.size();
	}
	std::string Packet::getData() const
	{
		if (data.empty()) return "";
		return data;
	}

	json Packet::CreateAnswer() const
	{
		return
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",0}, // Was 2 // Type Of Packet
					{"_R",0} // ID Recipient
				}
			},
			{"data",
				{
					// The Property Of Following Data
					{"_i",""}, // Id Of Packet
					{"_o",0}, // Orig Size To Decompress (If Was Decompressed)

					{"body", // All Data Is Here
						{
							{"_0",""},
						}
					}
				}
			}
		};
	}
	json Packet::CreateMessage() const
	{
		return
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",0}, // Was 2 // Type Of Packet
					{"_R",0} // ID Recipient
				}
			},
			{"data",
				{
					// The Property Of Following Data
					{"_i",""}, // Id Of Packet (Needs To Be In MD5)
					{"_o",0}, // Orig Size To Decompress (If Was Decompressed)

					{"body", // All Data Is Here
						{
							{"_0",""},
						}
					}
				}
			}
		};
	}
	json Packet::CreateMySQL() const
	{
		return
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",0}, // Was 2 // Type Of Packet
					{"_R",0} // ID Recipient
				}
			},
			{"data",
				{
					// The Property Of Following Data
					{"_i",""}, // Id Of Packet (Needs To Be In MD5)
					{"_o",0}, // Orig Size To Decompress (If Was Decompressed)

					{"body", // All Data Is Here
						{
							{"_0",""},
							{"_1",""}, // Needs To Be In MD5 (If It's A Password!)
						}
					}
				}
			}
		};
	}
	json Packet::CreateDisconnect() const
	{
		return
		//json Ret =
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",0}, // Was 2 // Type Of Packet
					{"_R",0} // ID Recipient
				}
			},
			{"data",
				{
					// The Property Of Following Data
					{"_i",""}, // Id Of Packet (Needs To Be In MD5)
					{"_o",0}, // Orig Size To Decompress (If Was Decompressed)

					{"body", // All Data Is Here
						{
							{"_0",""},
						}
					}
				}
			}
		};

		//_H.Settings = Ret["header"]["_s"].get<uint8_t>();
		//_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
		//	? Ret["data"].at("_o").get<size_t>()
		//	: 0u;
		//_H.type = (Type)Ret["header"].at("_t").get<size_t>();
	}
	std::string Packet::onSend()
	{
		//if (size >= 1024)
		//{
		//	// Parse All Packet (Include Header!)
		//	json js = json::parse(getData());
		//	std::string Data = js["data"]["body"].dump();

		//	// Compute Size ONLY Data From Our JSON
		//	size_t NewSize = Data.size();

		//	// Compress ONLY Data Or Body
		//	outData = new char[NewSize + 2];
		//	size = LZ4_compress_default(Data.c_str(), const_cast<char *>(outData), NewSize, NewSize + 2);

		//	if (size > 0)
		//	{
		//		// Original Data Size To Decompress
		//		js["data"]["_o"] = NewSize;

		//		// Set Flag That It Was Compressed
		//		js["header"]["_s"] = (js["header"]["_s"].get<uint8_t>() & Packet::Header::Compressed);

		//		// Put It Back
		//		js["data"]["body"] = outData;

		//		// Return Packet JSON With Compressed Data Block
		//		outData = js.dump().c_str();
		//	}
		//	else
		//	{
		//		printf("Something Is Wrong With Compress Data!");
		//		return nullptr;
		//	}
		//}
		return getData();
	}
	Packet *Packet::onReceive(const char *_data)
	{
		try
		{
			std::string NewData = _data;
			if (NewData.empty() || NewData.find("header") == std::string::npos ||
				NewData.rfind('#') == std::string::npos) return this;

			NewData.pop_back(); // Remove '#' From Back!
			json js = json::parse(NewData);

			if (js.empty()) return this;

			_H.Settings = js["header"].at("_s").get<uint8_t>();
			_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
				? js["data"].at("_o").get<size_t>()
				: 0u;
			_H.type = (Type)js["header"].at("_t").get<size_t>();

			if (_H.Settings & Header::TypeSettings::Compressed)
			{
				size_t Size = js["data"]["body"].dump().size();
				const char *Data = js["data"]["body"].dump().c_str();

				char* outData = new char[_H.OrigSize * 2];
				uint32_t outSize = LZ4_decompress_safe(Data, outData, /*_H.OrigSize*/Size, _H.OrigSize * 2);
				outData[outSize] = '\0';

				js["data"]["body"] = outData;
			}

			data = js["data"]["body"].dump();
		}
		catch (const json::parse_error &err)
		{
			std::cout << err.what() << std::endl;
		}

		return this;
	}

	void Packet::FillIn(json NewData)
	{
		if (NewData.empty())
			return;

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;

		data = NewData.dump() + '#';
	}
	void Packet::FillIn(Header NewHeader, json NewData)
	{
		if (NewData.empty())
			return;
		_H.type |= NewHeader.type;

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;

		data = NewData.dump() + '#';
	}

//	void Packet::send(std::shared_ptr<TCPSocket> socket)
//	{
//		if (!socket) return;
//		send(socket->getSocket());
//	}
//	void Packet::send(tcp::socket &socket)
//	{
//		std::size_t length = socket.write_some(asio::buffer(data, data.length()));
//#if defined (_SERVER) && defined (_CONSOLE)
//		std::cout << " Sent:\nData: " << data << ", numBytes: " << length << "\n";
//#endif
//	}
//	void Packet::receive(std::shared_ptr<TCPSocket> socket)
//	{
//		if (!socket) return;
//		receive(socket->getSocket());
//	}
//
//	void Packet::receive(tcp::socket &socket)
//	{
//		char *newdata = new char[2048 * 5];
//		std::error_code ec;
//		std::size_t length = 0;
//		length = socket.read_some(asio::buffer(newdata, 2048 * 5), ec);
//		newdata[length] = '\0';
//
//		if (ec)
//		{
//			if (ec != asio::error::eof)
//			{
//				std::cerr << "read_until error: " << ec.message() << std::endl;
//				socket.close();
//			}
//#ifndef NDEBUG
//			else
//			{
//				std::cout << "Control connection closed by client." << std::endl;
//				socket.close();
//			}
//#endif // !NDEBUG
//			std::cerr << ec.message() << std::endl;
//			socket.close();
//			return;
//		}
//		onReceive(newdata);
//#if defined (_SERVER) && defined (_CONSOLE)
//		std::cout << "\nServer Got New Packet:\nData: " << (const char*)newdata << ", numBytes: " << length << "\n";
//#endif
//	}
}