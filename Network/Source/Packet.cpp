#include "Packet.hpp"
#include <fstream>
#include <iostream>

namespace swl
{
	Packet::Packet() : readPos(0), data{}
	{
	}
	Packet::~Packet()
	{
	}
	template <typename T>
	Packet& Packet::operator <<(const T& data)
	{
		append(&data, sizeof(T));
		return *this;
	}
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
		*this << data.size();
		append(data.data(), sizeof(T) * data.size());
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
		data.clear();
		readPos = 0;
	}
	void Packet::resize(const uint32_t& size)
	{
		data.resize(size);
	}
	uint32_t Packet::getSize() const
	{
		return static_cast<uint32_t>(data.size());
	}
	void* Packet::getData()
	{
		return data.data();
	}
	void Packet::append(const void* _data, const uint32_t& size)
	{
		data.insert(data.end(), (char*)_data, (char*)_data + size);
	}
	Packet& Packet::operator <<(const std::string& _data)
	{
		*this << (uint32_t)_data.size();
		append(_data.data(), _data.size());
		return *this;
	}
	Packet& Packet::operator >>(std::string& _data)
	{
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
	const void* Packet::onSend(std::uint32_t& size)
	{
		size = getSize();
		return getData();
	}
	void Packet::onReceive(const void* _data, const std::uint32_t& size)
	{
		append(_data, size);
	}
}