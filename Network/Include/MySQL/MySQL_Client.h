#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H


///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL_Config.h"			 //	
									 //
#include "MySQL_Database.h"			 //
									 //
#include <map>						 //
#include <vector>					 //
#include <sstream>					 //
									 //
///////////////////////////////////////


namespace mysql
{
	class Impl;
	class Client
	{
	public:
		enum Status
		{
			Done,
			Error
		};

		///////////////////////////////////////////////
		// Constructor                               //
		///////////////////////////////////////////////

		Client();


		///////////////////////////////////////////////
		// Destructor				                 //
		///////////////////////////////////////////////

		~Client();


		///////////////////////////////////////////////
		// Methods                                   //
		///////////////////////////////////////////////

		void Connect(const std::string &user, const std::string &password, const std::string &host,
			const std::string DB = "", const unsigned short& port = 3306, const std::string charset = "utf8");

		std::list<std::pair<std::string, nlohmann::json>> TrySelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {});

		void TryInsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values, const std::vector<std::string> &condition = {});

		void Disconnect();
	private:
		std::shared_ptr<mysql::Impl> impl;
	};


	/////////////////////////////////////////////////////////////////////////////////
	// Definition of the most common names   	           	  	 				   //
	/////////////////////////////////////////////////////////////////////////////////

	typedef Client CLIENT;
	typedef Client MYSQLCLIENT;

} // namespace mysql


#endif // MYSQL_CLIENT_H
