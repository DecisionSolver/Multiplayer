#pragma once
#include "pch.h"
#include <SFML/Network/Packet.hpp>

namespace network
{
	class Packet
	{
	public:
		enum class Type: int
		{
			Chat = (1 << 0),
			Login = (1 << 1), // Was MySQL
			Connection = (1 << 2),
			Disconnection = (1 << 3),
			PlaySound = (1 << 4),
			// Used When Sending Mic Captured Packet
			VOIP = (1 << 5), // To understand it's VOIP Type Of Packet (it needs to proccess it a bit different)

			// Server
			ClosedServerByUpdate = (1 << 6),
			// Getting List Of Users Who's Online In The Moment (Information Based On MySQL)
			GetListUsersOnline = (1 << 7),

			Sync_PosChanges = (1 << 8),
			Sync_RotChanges = (1 << 9),
			Sync_SclChanges = (1 << 10),

			Sync_NewNodeName = (1 << 11),

			// First of all we send this packet to check who has this file in resources.
			// If not then client (who has this file) will send the file to FTP and share
			// with this file with users who haven't it.
			//
			// Packet should have data like this
			//
			// "Sync_File":
			// {
			//		"File_Name.obj":
			//		{
			//			"id_who_sent_it": 1,
			//		},
			//		...
			// }
			//
			// "id_who_sent_it" needs to know who sent it 'cause it uses for indicate where the file has.
			// It means that user who shares this file is uploaded it to the FTP folder and we can get it
			// with that ID (and file name).
			//
			// Note: Sync_File Needs ONLY For Upload And Detected That Server Will Start Asking Users About This File.
			// Sync_File_Sync Needs ONLY For Answering If Have File Or Not!
			//
			Sync_File = (1 << 12),
			Sync_NewNode = (1 << 13),

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
			Get_MetaData_Project_Ex = (1 << 14),

			// JSON e.g. Name Each GO (count + ID)
			//
			// {
			//		"First GO":
			//		{
			//			...
			//		},
			//		...
			// }
			Get_MetaData_Project = (1 << 15),

			// Get All The Projects From The Server (If User Has Connect To)
			Get_AllProjects = (1 << 16),

			// This Will Work If User Has Rights To This!
			Make_Commit = (1 << 17),
			
			// It Indicates And Need To Block Everything About Change The Level (Project)
			// Unblock Will After Full-Commiting Is Done
			IsCommiting = (1 << 18), IsCommitingDone = (1 << 19), IsCommitingFailed = (1 << 20),

			Ping = (1 << 21), // Echo Packet

			NONE = -1
		};
		enum class Status: int
		{
			OK = (1 << 0),

			// Log in Statuses
			Not_Found = (1 << 1), // If User Wasn't Found On Server
			Already_Online = (1 << 2), // When User Already In Online
			Need_To_LogIn = (1 << 3), // When Cookies Wasn't Found On Server (Need To Send Log&Pass)

			// Disconnection Status
			TimeOut_LogIn = (1 << 4), // When Server Has Out Of Time For Login Packet
			
			// Only If Cookie System Is Enabled
			// Sends From Server When User Directly Connect To Without Cookies
			NotAllowWithoutCookie = (1 << 5),

			NONE = -1
		};

		struct Header
		{
			enum TypeSettings
			{
				Compressed = 1,
			};
			uint8_t Settings = 0; // IsCompressed etc...
			int type = (int)Packet::Type::NONE;
			bool IsAnswer = false; // Sets Only When Comes In onReceive Function

			// Came ONLY From Server! (-1 By Default Means Non User!)
			int ID_Receiver = -1; // ID From MySQL Line Of User Row Easy To Get Data
			size_t OrigSize = 0u;

			Header(int NewType): type(NewType), Settings(0) {}
			Header(int NewType, uint8_t NewSettings): type(NewType), Settings(NewSettings) {}
			Header() = default;
		};

		virtual ~Packet()
		{
			BinaryData.clear();
		}

		void clear();
		size_t getSize() const;
		nlohmann::json &getData();

		// Used In VOIP Type Of Packets
		const void *getRAWData(bool Need2Plus = false)
		{
			if (Need2Plus)
				return static_cast<const sf::Int16 *>(BinaryData.getData()) + readPos;
			else
				return static_cast<const sf::Int16 *>(BinaryData.getData());
		}

		// Filling data
		void FillIn(network::Packet::Header NewHeader, std::stringstream &Data);
		void FillIn(const nlohmann::json &Data);
		void FillIn(const network::Packet::Header &NewHeader, const nlohmann::json &Data);
		
		// When Send Packet (From JSON To VOIP (sf::Packet))
		void PrepareForVOIP();
		// When Receive Packet (From VOIP (sf::Packet) To JSON)
		void FromVOIP();

		network::Packet *operator =(network::Packet &pack)
		{
			BinaryData = pack.GetBinaryData();
			readPos = pack.GetBinaryData().getReadPosition();

			JSON_Data = pack.JSON_Data;
			_H = pack._H;
			return this;
		}

		operator bool();
		network::Packet::Header getHeader() const { return _H; }

		network::Packet *CreatePacket(const int &Type, const bool &isAnswer = true, const nlohmann::json &Data = {});
		
		network::Packet *onReceive(std::stringstream &Data);

		// Used In VOIP Type Of Packets
		void onReceive(const void* Data, std::size_t size) { BinaryData.append(Data, size); }
		// Used In VOIP Type Of Packets
		std::size_t getDataSize(bool Need2Minus = false)
		{
			if (Need2Minus)
				return BinaryData.getDataSize() - readPos;
			else
				return BinaryData.getDataSize();
		}
	
		sf::Packet GetBinaryData() { return BinaryData; }
	private:
		network::Packet::Header _H;
		nlohmann::json JSON_Data = {};
		nlohmann::json onSend();
		std::size_t readPos = 0; // To Correct When We Use "operator ="

		sf::Packet BinaryData = sf::Packet();
	};
}