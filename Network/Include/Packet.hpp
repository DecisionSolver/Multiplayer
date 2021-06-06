#pragma once
#include "pch.h"

// for convenience
using json = nlohmann::json;
namespace network
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
			PlayVoice,

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
			size_t OrigSize = 0u;

			Header(Type NewType): type(NewType), Settings(0) {}
			Header(Type NewType, uint8_t NewSettings): type(NewType), Settings(NewSettings) {}
			Header() {}
		};

		Packet() {}
		Packet(const Packet& from)
		{
			data << from.data.rdbuf();
			_H = from._H;
		}
		virtual ~Packet() {}

		void clear();
		size_t getSize() const;
		std::stringstream &getData();

		// Filling data
		void FillIn(std::stringstream &Data);
		void FillIn(Header NewHeader, std::stringstream &Data);
		void FillIn(const json &Data);
		void FillIn(const Header &NewHeader, const json &Data);

		Packet *operator =(Packet &pack)
		{
			data << pack.data.rdbuf();
			_H = pack._H;
			return this;
		}

		operator bool();
		Header getHeader() const { return _H; }

		Packet *CreateAnswer();
		Packet *CreateMessage();
		Packet *CreateMySQL();
		Packet *CreateDisconnect();
		
		Packet *onReceive(std::stringstream &Data);
	protected:
		friend TCPSocket;
		friend UDPSocket;
		std::stringstream &onSend();
	private:
		Header _H;
		std::stringstream data;

		json Message, MySQL_Request, Answer_Request;
	};
}