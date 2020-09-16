#include "Packet.hpp"
#include <fstream>
#include <iostream>

namespace swl
{
	//template <typename T>
	//Packet& Packet::operator <<(const T& data)
	//{
	//	append(const_cast<const void *>(data), sizeof(T));
	//	return *this;
	//}
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

		//char *NewString = data.data();
		//NewString[data.size()] = '\0';
		return data.c_str();
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
		*this << (uint32_t)_data.size();
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
	Packet& Packet::operator <<(swl::File& file)
	{
		*this << file.getFileName();
		*this << file.getDataSize();
		append(file.getFileData(), file.getDataSize());
		return *this;
	}
	Packet& Packet::operator >>(swl::File& file)
	{
		std::string tempName;
		std::vector<char> tempData;
		*this >> tempName;
		file.setFileName(tempName);
		*this >> tempData;
		file.setFileData(tempData);
		return *this;
	}

#include "LZ4/lz4.h"
	const char* Packet::onSend(std::uint32_t& size)
	{
		size = getSize();
		//json j = getData();
		return getData();
		//char* outData = new char[getSize() + 2];
		//size = LZ4_compress_default((const char*)getData(), outData, getSize(), getSize() + 2);
		//return outData;

		//std::cout << j << std::endl;
		//return j.get_ptr<json::object_t*>();
	}
	void Packet::onReceive(const char* _data, const std::uint32_t& size)
	{
		try
		{
			if (_data[0] == '\n' || size == 0) return;
			json js = json::parse(_data);
			if (js.empty())
				DebugBreak();

			_H.Settings = js["header"].at("_s").get<uint8_t>();
			_H.OrigSize = _H.Settings & Header::TypeSettings::IsCompressed
				? js["data"].at("_o").get<size_t>()
				: 0u;
			_H.type = (Type)js["header"].at("_t").get<size_t>();
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
			//char* outData = new char[size * 2];
			//uint32_t outSize = LZ4_decompress_safe((const char*)NewData, outData, size, size * 2);
		//}
		////////////////////////////////////////////////////
	}

	void Packet::FillIn(const json NewData)
	{
		if (NewData.empty())
			return;

		_H.Settings = NewData["header"].at("_s").get<uint8_t>();
		_H.OrigSize = _H.Settings & Header::TypeSettings::IsCompressed
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
	void Packet::FillIn(Header NewHeader, const void *NewData)
	{
		if (!NewData || !NewHeader.type)
			return;

		_H = NewHeader;

		json RequestJSON = (char *)NewData;
		if (!RequestJSON.is_structured())
		{
			RequestJSON =
			{
				{"header",
				   {
					   { "_s",NewHeader.Settings}, // Settings
					   {"type",NewHeader.type} // Type Of Packet
				   }
				},
				{"data",
					{
						{ // The Main Data
							{"id","trgffdsfh"}, // Id Of Packet (Needs To Be In MD5)
							{"_o",NewHeader.Settings & Header::TypeSettings::IsCompressed 
							? NewHeader.OrigSize
							: 0}, // Orig Size To Decompress
						},
						{"body", // All Data Is Here
							{
								{"_0","Login: PBAX"},
								{"_1", "Pass: _SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
							}
						}
					}
				}
			};
		}

		//auto NewPacket = json::to_msgpack(RequestJSON);
		//append(NewPacket.data(), NewPacket.size());
	}
}
