#pragma once
#include "pch.h"

// for convenience
using json = nlohmann::json;
namespace swl
{
	class TCPSocket;
	class UDPSocket;
	class Packet: public std::enable_shared_from_this<Packet>
	{
	public:
		enum Type
		{
			Chat = 0,
			File,
			MySQL,
			Answer,
			Connection,
			Disconnection,
			PlaySound,

			// Server
			ClosedServerByUpdate,
			// From Server
			GetListUsersOnline
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
	
			Header(Type NewType): type(NewType), Settings(0) {}
			Header(Type NewType, uint8_t NewSettings): type(NewType), Settings(NewSettings) {}
			Header() {}
		};

		Packet() {}
		Packet(const Packet& from)
		{
			data = from.data;
			_H = from._H;
		}
		virtual ~Packet() {}

		void clear();
		size_t getSize() const;
		std::string getData() const;

		// Filling data
		void FillIn(json NewData);
		void FillIn(Header NewHeader, json NewData);

		//void send(std::shared_ptr<TCPSocket> socket);
		//void send(tcp::socket &socket);
		//void receive(std::shared_ptr<TCPSocket> socket);
		//void receive(tcp::socket &socket);

		operator bool() { return !data.empty(); }
		Header getHeader() { return _H; }

		json CreateAnswer() const;
		json CreateMessage() const;
		json CreateMySQL() const;
		json CreateDisconnect() const;
		
		Packet *onReceive(const char *_data);
	protected:
		friend TCPSocket;
		friend UDPSocket;
		std::string onSend();
	private:
		Header _H;
		std::string data;

		json Message, MySQL_Request, Answer_Request;
	};
}