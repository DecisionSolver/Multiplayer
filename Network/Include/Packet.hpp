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
			Connection,
			Disconnection,
			PlaySound,

			// Server
			ClosedServerByUpdate,
			// From Server
			GetListUsersOnline,

			Sync_PosChanges,
			Sync_RotChanges,
			Sync_SclChanges,

			Sync_NewNodeName,
			Sync_NewNode,

			// JSON e.g. Name Project, Name Each GO (count + ID), Each GO Parameter (transformations, etc)
			// "New Project":
			// {
			//		"First GO":
			//		{
			//			"transformations": {
			//				"pos": [0,0,0],
			//				"rot": [0,0,0],
			//				"scl": [0,0,0]
			//			},
			//		},
			//		...
			// }
			Get_MetaData_Project_Ex,

			// JSON e.g. Name Each GO (count + ID)
			//
			// {
			//		"First GO":
			//		{
			//			...
			//		},
			//		...
			// }
			Get_MetaData_Project
		};

		struct Header
		{
			enum TypeSettings
			{
				Compressed = 1,
			};
			uint8_t Settings = 0; // IsCompressed etc...
			int type;
			bool IsAnswer = false; // Sets Only When Comes In onReceive Function
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

		operator bool() { return !data.empty(); }
		Header getHeader() { return _H; }

		Packet *CreateAnswer();
		Packet *CreateMessage();
		Packet *CreateMySQL();
		Packet *CreateDisconnect();
		
		Packet *onReceive(std::string NewData);
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