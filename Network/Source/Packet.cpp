#include "pch.h"
#include "Packet.hpp"
#include <fstream>
#include <iostream>
#include "LZ4/lz4.h"

namespace network
{
	void Packet::clear()
	{
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
	json Packet::getData()
	{
		return data;
	}

	Packet *Packet::CreatePacket(const Packet::Type &Type, const bool &isAnswer, const nlohmann::json &Data)
	{		
		json Return =
		{
			{"header",
				{
					{"_s",0}, // Settings
					{"_t",(size_t)Type}, // Was 2 // Type Of Packet
					{"_A",isAnswer}, // If Is It Answer?
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
						}
					}
				}
			}
		};

		if (Data.is_null() && Data.empty())
			Return["data"]["body"] = json::object({ { "_0", "" }, { "_1", "" } });
		else
		{
			nlohmann::json::const_iterator data_beg, data_end;
			if (Data.find("data") != Data.end() && Data.find("body") != Data.end())
			{
				data_beg = Data["data"]["body"].begin();
				data_end = Data["data"]["body"].end();
			}
			else
			{
				data_beg = Data.begin();
				data_end = Data.end();
			}
			Return["data"]["body"] = { data_beg, data_end };
		}
		_H.Settings = Return["header"]["_s"].get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? Return["data"]["_o"].get<size_t>()
			: 0u;
		_H.type = Return["header"]["_t"].get<size_t>();
		data = Return;

		return this;
	}
	json Packet::onSend()
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
	Packet *Packet::onReceive(std::stringstream &Data)
	{
		try
		{
			json js = json::parse(Data);

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
				const char *_Data = js["data"]["body"].dump().c_str();

				char* outData = new char[_H.OrigSize * 2];
				uint32_t outSize = LZ4_decompress_safe(_Data, outData, /*_H.OrigSize*/Size, _H.OrigSize * 2);
				outData[outSize] = '\0';

				js["data"]["body"] = outData;
			}

			data = js["data"]["body"];
		}
		catch (const json::parse_error &err)
		{
#if defined(HAS_LOGGER)
			Logger_Error_F("json::parse_error Failed: %s\n", err.what());
#endif
			return new Packet();
		}

		return this;
	}

	void Packet::FillIn(const Header &NewHeader, const json &Data)
	{
		std::stringstream NewData;
		NewData << Data;

		FillIn(NewHeader, NewData);
	}
	void Packet::FillIn(const json &Data)
	{
		json NewData = Data;		
		if (NewData.empty())
			return;

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;

		data = NewData;
	}
	void Packet::FillIn(Header NewHeader, std::stringstream &Data)
	{
		json NewData;
		Data >> NewData;

		if (NewData.empty())
			return;
		_H.type = NewHeader.type;

		try
		{
			// Translate String To Array JSON
			if (NewData.is_string() && !NewData.empty())
				NewData = json::parse(NewData.get<json::string_t>());
		}
		catch (const json::parse_error &err)
		{
#if defined(HAS_LOGGER)
			Logger_Error_F("json::parse_error Failed: %s\n", err.what());
#endif
			return;
		}

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;
		NewData["header"]["_A"] = _H.IsAnswer;

		data = NewData;
	}
	Packet::operator bool()
	{
		return !data.empty() && data.dump() != "null";
	}
}