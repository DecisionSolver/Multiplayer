
#ifndef MULTIPLAYER_ANSWER_H
#define MULTIPLAYER_ANSWER_H


////////////////////////////////////////////////////
// Headers			    					      //
////////////////////////////////////////////////////
												  //
#include "../../Network/DataPacket/DataPacket.h"  //
												  //
#include <windows.h>							  //
#include <string>								  //
												  //
////////////////////////////////////////////////////


namespace mp
{
	class Answer
	{
		private:

			///////////////////////////////////////////////
			// Friendly class							 //
			///////////////////////////////////////////////

			friend class TcpServer;


		public:
		
			///////////////////////////////////////////////
			// Data type								 //
			///////////////////////////////////////////////

			enum Type
			{	
				None,
				Success,
				Error
			};


		private:
	
			///////////////////////////////////////////////
			// Member data 								 //
			///////////////////////////////////////////////
		
			Type type = None;
			std::string error_message;
	

		public:

			///////////////////////////////////////////////
			// Constructors								 //
			///////////////////////////////////////////////

			Answer();


		private:

			///////////////////////////////////////////////
			// Private constructors						 //
			///////////////////////////////////////////////

			Answer(Type type);

			Answer(std::string error_message);
		

		public:

			///////////////////////////////////////////////
			// Method	 								 //
			///////////////////////////////////////////////

			bool IsSuccess();
		
			Type GetType();

			std::string GetErrorMessage();

			void Clear();
	

		public:
		
			///////////////////////////////////////////////
			// Friendly operator overloading	         //
			///////////////////////////////////////////////

			friend net::DataPacket& operator <<(net::DataPacket& packet, Answer answer);

			friend net::DataPacket& operator >>(net::DataPacket& packet, Answer& answer);
	};

} // namespace mp



#endif // MULTIPLAYER_ANSWER_H