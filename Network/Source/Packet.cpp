#include "pch.h"
#include "Packet.hpp"
#include <fstream>
#include <iostream>
//#include "LZ4/lz4.h"

namespace network
{
	void Packet::clear()
	{
		JSON_Data.clear();
		_H = Packet::Header();

		BinaryData.clear();
	}
	size_t Packet::getSize() const
	{
		return Packet::JSON_Data.size();
	}
	nlohmann::json &Packet::getData()
	{
		return Packet::JSON_Data["data"]["body"];
	}

	Packet *Packet::CreatePacket(const int &Type, const bool &isAnswer, const nlohmann::json &Data)
	{
		nlohmann::json Return =
		{
			{"header",
				{
					//_s_t_A_R // illuminati
					{"_s",0}, // Settings
					{"_t",Type}, // Type Of Packet
					{"_A",isAnswer}, // If Is It Answer?
					{"_R",-1} // ID Recipient
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

		if (!(Type & (int)network::Packet::Type::VOIP))
		{
			if (Data.is_null() && Data.empty())
				Return["data"]["body"] = nlohmann::json::object({ { "_0", "" }, { "_1", "" } });
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
		}
		_H.Settings = Return["header"]["_s"].get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? Return["data"]["_o"].get<size_t>()
			: 0u;
		_H.type = Type;
		_H.IsAnswer = isAnswer;

		if (Data.find("header") != Data.end() && (Data.find("_R") != Data.end()))
			_H.ID_Receiver = Data["header"]["_R"].get<nlohmann::json::boolean_t>();

		Packet::JSON_Data = Return;

		return this;
	}
	nlohmann::json Packet::onSend()
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
			nlohmann::json js = nlohmann::json::parse(Data);

			if (js.empty() || js.find("header") == js.end()) return this;

			auto End = js["header"].end();
			if (js["header"].find("_s") != End)
				_H.Settings = js["header"]["_s"].get<uint8_t>();

			if (js["header"].find("_o") != End)
				_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
				? js["data"]["_o"].get<size_t>()
				: 0u;
			if (js["header"].find("_t") != End)
				_H.type = js["header"]["_t"].get<int>();
			if (js["header"].find("_A") != End)
				_H.IsAnswer = js["header"]["_A"].get<nlohmann::json::boolean_t>();

			if (js["header"].find("_R") != End)
				_H.ID_Receiver = (int)js["header"]["_R"].get<nlohmann::json::number_integer_t>();

			if (_H.Settings & Header::TypeSettings::Compressed)
			{
				size_t Size = js["data"]["body"].dump().size();
				const char *_Data = js["data"]["body"].dump().c_str();

				char* outData = new char[_H.OrigSize * 2];
				//uint32_t outSize = LZ4_decompress_safe(_Data, outData, /*_H.OrigSize*/Size, _H.OrigSize * 2);
				//outData[outSize] = '\0';

				js["data"]["body"] = outData;
			}

			if (js.find("data") != js.end())
			{
				End = js["data"].end();
				if (js["data"].is_object() && js["data"].find("body") != End)
					Packet::JSON_Data = js["data"]["body"];
			}
		}
		catch (const nlohmann::json::parse_error &err)
		{
#if __has_include("logger.h")
			Logger_Error_F("json::parse_error Failed: {}\n", err.what());
#endif
			return new Packet();
		}

		return this;
	}

	void Packet::FillIn(const Header &NewHeader, const nlohmann::json &Data)
	{
		std::stringstream NewData;
		NewData << Data;

		FillIn(NewHeader, NewData);
	}
	void Packet::PrepareForVOIP()
	{
		if (Packet::JSON_Data.empty() || Packet::JSON_Data.find("header") == Packet::JSON_Data.end() ||
			(Packet::JSON_Data["header"].find("_t") == Packet::JSON_Data["header"].end() ||
				Packet::JSON_Data["header"].find("_A") == Packet::JSON_Data["header"].end() ||
				Packet::JSON_Data["header"].find("_R") == Packet::JSON_Data["header"].end() ||
				Packet::JSON_Data.find("data") == Packet::JSON_Data.end() ||
				Packet::JSON_Data["data"].find("body") == Packet::JSON_Data["data"].end()))
			return;

		(BinaryData) << (Packet::JSON_Data["header"]["_t"].get<nlohmann::json::number_integer_t>());
		(BinaryData) << ((bool)Packet::JSON_Data["header"]["_A"].get<nlohmann::json::boolean_t>());

		(BinaryData) << (Packet::JSON_Data["header"]["_R"].get<nlohmann::json::number_integer_t>());

		// In Source Data Of VOIP It's Never Will Happen!
		if (Packet::JSON_Data["data"]["body"].size() > 0)
		{
			(BinaryData) << ((sf::Uint64)Packet::JSON_Data["data"]["body"].size());

			for (auto it = Packet::JSON_Data["data"]["body"].begin(); it != Packet::JSON_Data["data"]["body"].end(); ++it)
			{
				if ((*it).type() == nlohmann::json::value_t::number_float)
				{
					(BinaryData) << ("f");
					(BinaryData) << ((float)(*it));
				}
				else if ((*it).type() == nlohmann::json::value_t::number_integer)
				{
					(BinaryData) << ("i");
					(BinaryData) << ((sf::Int64)(*it));
				}
				else if ((*it).type() == nlohmann::json::value_t::number_unsigned)
				{
					(BinaryData) << ("u");
					(BinaryData) << ((sf::Uint64)(*it));
				}
				else if ((*it).type() == nlohmann::json::value_t::boolean)
				{
					(BinaryData) << ("b");
					(BinaryData) << ((bool)(*it));
				}
				else
				{
					(BinaryData) << ("s");
					(BinaryData) << ((*it).dump());
				}
			}
		}
	}
	void Packet::FromVOIP()
	{
		bool _A = false;
		nlohmann::json::number_integer_t _t = (int)network::Packet::Type::VOIP, _R = 0;
		nlohmann::json ReceivedData;

		(BinaryData) >> (_t);

		(BinaryData) >> (_A);
		(BinaryData) >> (_R);
		_H.ID_Receiver = (int)_R;

		if (_t > (int)network::Packet::Type::Ping || _t <= (int)network::Packet::Type::NONE)
		{
			_t = (int)network::Packet::Type::VOIP;
			(void)CreatePacket((int)_t, _A, ReceivedData);
			return;
		}

		// If It's Captured Packet We No Need To Proccess It Here! (Dead-Loop)
		if (_t != (int)network::Packet::Type::VOIP)
		{
			sf::Uint64 dataCount = 0;
			(BinaryData) >> (dataCount);
			for (size_t i = 0; i < dataCount; i++)
			{
				std::string type;
				(BinaryData) >> (type);
				if (type.empty()) break;

				if (type == "b")
				{
					bool value;
					(BinaryData) >> (value);
					ReceivedData["_" + std::to_string(i)] = value;
				}
				else if (type == "f")
				{
					float value;
					(BinaryData) >> (value);
					ReceivedData["_" + std::to_string(i)] = value;
				}
				else if (type == "i")
				{
					sf::Int64 value;
					(BinaryData) >> (value);
					ReceivedData["_" + std::to_string(i)] = (nlohmann::json::number_integer_t)value;
				}
				else if (type == "u")
				{
					sf::Uint64 value;
					(BinaryData) >> (value);
					ReceivedData["_" + std::to_string(i)] = (nlohmann::json::number_unsigned_t)value;
				}
				else if (type == "s")
				{
					std::string value;
					(BinaryData) >> value;
					ReceivedData["_" + std::to_string(i)] = nlohmann::json::parse(value);
				}
			}
		}
		(void)CreatePacket((int)_t, _A, ReceivedData);
	}
	void Packet::FillIn(const nlohmann::json &Data)
	{
		nlohmann::json NewData = Data;
		if (NewData.empty())
			return;

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;

		JSON_Data = NewData;
	}
	void Packet::FillIn(Header NewHeader, std::stringstream &Data)
	{
		nlohmann::json NewData;
		Data >> NewData;

		if (NewData.empty())
			return;
		_H.type = NewHeader.type;

		try
		{
			// Translate String To Array JSON
			if (NewData.is_string() && !NewData.empty())
				NewData = nlohmann::json::parse(NewData.get<nlohmann::json::string_t>());
		}
		catch (const nlohmann::json::parse_error &err)
		{
#if __has_include("logger.h")
			Logger_Error_F("json::parse_error Failed: {}\n", err.what());
#endif
			return;
		}

		NewData["header"]["_s"] = _H.Settings;
		NewData["data"]["_o"] = _H.OrigSize;
		NewData["header"]["_t"] = _H.type;
		NewData["header"]["_A"] = _H.IsAnswer;

		JSON_Data = NewData;
	}
	Packet::operator bool()
	{
		return (!JSON_Data.empty() && JSON_Data.dump() != "null") || BinaryData.getDataSize() > 0;
	}
}
