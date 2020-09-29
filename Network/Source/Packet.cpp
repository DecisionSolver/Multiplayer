#include "Packet.hpp"
#include <fstream>
#include <iostream>

namespace swl
{
	template <typename T>
	Packet& Packet::operator >>(T& _data)
	{
		if (readPos + sizeof(T) <= data.size())
		{
			_data = *reinterpret_cast<T*>(&data[readPos]);
			readPos += sizeof(T);
		}
		return *this;
	}
	template <typename T>
	Packet& Packet::operator <<(const std::vector<T>& _data)
	{
		*this << _data.size();
		append(_data.data(), sizeof(T) * _data.size());
		return *this;
	}
	template <typename T>
	Packet& Packet::operator >>(std::vector<T>& _data)
	{
		uint32_t size = 0;
		*this >> size;
		if (readPos + sizeof(T)*size <= data.size())
		{
			_data.resize(size);
			memcpy(_data.data(), data.data() + readPos, sizeof(T) * size);
			readPos += sizeof(T)* size;
		}
		return *this;
	}
	void Packet::clear()
	{
		if (!data.empty())
			data.clear();
		readPos = 0;
	}
	void Packet::resize(const uint32_t& size)
	{
		data.resize(size);
	}
	size_t Packet::getSize() const
	{
		return data.size();
	}
	char *Packet::getData()
	{
		return data.data();
	}
	const char *Packet::ToString()
	{
		if (data.empty())
			return "";

		char *NewString = new char[data.size()];
		for (size_t i = 0; i < data.size(); i++)
		{
			NewString[i] = data.at(i);
		}
		NewString[data.size()] = '\0';
		return NewString;
	}
	void Packet::append(const char* _data, const uint32_t& size)
	{
		for (size_t i = 0; i < size; i++)
		{
			data.push_back(_data[i]);
		}
		//data.insert(data.end(), _data, _data + size);
	}
	Packet& Packet::operator <<(const std::string& _data)
	{
		//*this << (uint32_t)_data.size();
		append(_data.data(), _data.size());
		return *this;
	}
	Packet& Packet::operator >>(std::string& _data)
	{
		if (data.empty())
			return *this;
		if (readPos + sizeof(uint32_t) <= data.size())
		{
			uint32_t stringSize = 0;
			*this >> stringSize;
			if (readPos + stringSize <= data.size())
			{
				_data.reserve(stringSize);
				_data.assign(&data[readPos], stringSize);
				readPos += stringSize;
			}
		}
		return *this;
	}

#include "LZ4/lz4.h"
	json Packet::CreateAnswer()
	{
		return
		{
			{"header",
				{
					{ "_s",0}, // Settings
					{"_t",3}, // Was 2 // Type Of Packet
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
	json Packet::CreateMessage()
	{
		return
		{
			{"header",
				{
					{ "_s",0}, // Settings
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
	json Packet::CreateMySQL()
	{
		return 
		{
			{"header",
				{
					{ "_s",0}, // Settings
					{"_t",2}, // Was 2 // Type Of Packet
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
	const char* Packet::onSend(std::uint32_t& size)
	{
		const char *outData = ToString();
		size = getSize();
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
		return outData;
	}
	void Packet::onReceive(const char* _data, const std::uint32_t& size)
	{
		try
		{
			if (std::string(_data).find("header") == std::string::npos
				|| _data == "" || _data[0] == '\0' || size == 0) return;
			json js = json::parse(_data);
		
			if (js.empty()) return;

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

			append(js.dump().c_str(), js.dump().size());
		}
		catch (const json::parse_error& err)
		{
			std::cout << err.what() << std::endl;
		}

		//ToDo("Decompress here");
		////////////////////////////////////////////////////
		//Decompression Prototype
		////////////////////////////////////////////////////
		//void* NewData = const_cast<void*>(_data);
		//auto OBJ = JSON.Parse(NewData);
		//h = OBJ._s;
		//if (h & (1 << 2)) 
		//{
		//}
		////////////////////////////////////////////////////
	}

	void Packet::FillIn(const json NewData)
	{
		if (NewData.empty())
			return;

		_H.Settings = NewData["header"].at("_s").get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::Compressed
			? NewData["data"].at("_o").get<size_t>()
			: 0u;
		_H.type = (Type)NewData["header"].at("_t").get<size_t>();

		auto NewPacket = NewData.dump();
		append(NewPacket.c_str(), NewPacket.size());
	}
	void Packet::FillIn(Header NewHeader, const json NewData)
	{
		if (NewData.empty())
			return;
		_H = NewHeader;

		auto NewPacket = NewData.dump();
		append(NewPacket.c_str(), NewPacket.size());
	}
}
