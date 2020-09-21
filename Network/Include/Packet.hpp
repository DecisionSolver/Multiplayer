#pragma once
#include <WinSock2.h>
#include "File.hpp"
#include <vector>
#include <string>
#include "nlohmann/json.hpp"

// for convenience
using json = nlohmann::json;
namespace swl
{
	class TCPSocket;
	class UDPSocket;
	class Packet
	{
	public:
		enum Type
		{
			Chat = 0,
			File,
			//Audio,
			MySQL,
			Answer
		};

		struct Header
		{
			enum TypeSettings
			{
				Compressed = 1,
			};
			uint8_t Settings = 0; // IsCompressed etc...
			Type type;
			size_t OrigSize = 0;

			Header(Type NewType, uint8_t NewSettings): type(NewType), Settings(NewSettings) {}
			Header() {}
		};
		Packet(): readPos(0) {}
		Packet(const Packet& from)
		{
			readPos = 0;
			data = from.data;
			_H = from._H;
		}
		virtual ~Packet() {}

		void clear();
		void resize(const uint32_t& size);

		size_t getSize() const;
		char *getData();
		const char *ToString();

		// Filling data
		void FillIn(const json NewData);
		void FillIn(Header NewHeader, const json NewData);
		void append(const char* _data, const uint32_t& size);
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
		operator bool() { return !data.empty(); }

		Header getHeader() { return _H; }

		json CreateAnswer();
		json CreateMessage();
		json CreateMySQL();
	protected:
		friend TCPSocket;
		friend UDPSocket;
		const char* onSend(std::uint32_t& size);
		void onReceive(const char* _data, const std::uint32_t& size);
	private:
		uint32_t readPos = 0;
		std::vector<char> data;
		Header _H;
		json Message, MySQL_Request, Answer_Request;
	};
}