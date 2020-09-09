#pragma once
#include <WinSock2.h>
#include "File.hpp"
#include <vector>
#include <string>

namespace swl
{
	class TCPSocket;
	class UDPSocket;
	class Packet
	{
	public:
		enum Type
		{
			Chat,
			File,
			Audio
		};
		Packet();
		virtual ~Packet();
		void clear();
		void resize(const uint32_t& size);
		uint32_t getSize() const;
		void* getData();
		void append(const void* _data, const uint32_t& size);
		template <typename T>
		Packet& operator <<(const T& _data);
		template <typename T>
		Packet& operator >>(T& _data);
		template <typename T>
		Packet& operator <<(const std::vector<T>& _data);
		template <typename T>
		Packet& operator >>(std::vector<T>& _data);
		Packet& operator <<(const std::string& _data);
		Packet& operator >>(std::string& _data);
		Packet& operator <<(swl::File& file);
		Packet& operator >>(swl::File& file);
	protected:
		friend TCPSocket;
		friend UDPSocket;
		virtual const void* onSend(std::uint32_t& size);
		virtual void onReceive(const void* _data, const std::uint32_t& size);
	private:
		uint32_t readPos = 0;
		std::vector<char> data;
	};
}