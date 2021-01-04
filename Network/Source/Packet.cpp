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
		_H.IsAnswer = false;
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

	Packet *Packet::CreateAnswer()
	{
		json Return =
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",0}, // Was 2 // Type Of Packet
					{"_A",true}, // If Is It Answer?
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

		_H.Settings = Return["header"]["_s"].get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? Return["data"]["_o"].get<size_t>()
			: 0u;
		_H.type = (Type)Return["header"]["_t"].get<size_t>();
		data = Return["data"]["body"].dump();
		_H.IsAnswer = Return["header"]["_A"].get<bool>();

		return this;
	}
	Packet *Packet::CreateMessage()
	{
		json Return =
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",Chat}, // Was 2 // Type Of Packet
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
		
		_H.Settings = Return["header"]["_s"].get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? Return["data"]["_o"].get<size_t>()
			: 0u;
		_H.type = (Type)Return["header"]["_t"].get<size_t>();
		data = Return["data"]["body"].dump();

		return this;
	}
	Packet *Packet::CreateMySQL()
	{
		json Return =
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",MySQL}, // Was 2 // Type Of Packet
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
		
		_H.Settings = Return["header"]["_s"].get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? Return["data"]["_o"].get<size_t>()
			: 0u;
		_H.type = (Type)Return["header"]["_t"].get<size_t>();
		data = Return["data"]["body"].dump();

		return this;
	}
	Packet *Packet::CreateDisconnect()
	{
		json Return =
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",Disconnection}, // Was 2 // Type Of Packet
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

		_H.Settings = Return["header"]["_s"].get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? Return["data"]["_o"].get<size_t>()
			: 0u;
		_H.type = (Type)Return["header"]["_t"].get<size_t>();
		data = Return.dump();

		return this;
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
	Packet *Packet::onReceive(std::string NewData)
	{
		try
		{
			if (NewData.empty() || NewData.find("header") == std::string::npos) return this;
			NewData.erase(NewData.find("#"), NewData.length());
			json js = json::parse(NewData);

			if (js.empty()) return this;

			auto End = js["header"].end();
			if (js["header"].find("_s") != End)
				_H.Settings = js["header"]["_s"].get<uint8_t>();

			if (js["header"].find("_o") != End)
				_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
				? js["data"]["_o"].get<size_t>()
				: 0u;
			if (js["header"].find("_t") != End)
				_H.type = (Type)js["header"]["_t"].get<int>();
			if (js["header"].find("_A") != End)
				_H.IsAnswer = js["header"]["_A"].get<json::boolean_t>();

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
		_H.type = NewHeader.type;

		// Translate String To Array JSON
		if (NewData.is_string() && !NewData.get<json::string_t>().empty())
			NewData = json::parse(NewData.get<json::string_t>());

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;
		NewData["header"]["_A"] = _H.IsAnswer;

		data = NewData.dump() + '#';
	}
}