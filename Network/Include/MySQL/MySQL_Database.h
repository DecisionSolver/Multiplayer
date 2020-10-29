
#ifndef MYSQL_DATABASE_H
#define MYSQL_DATABASE_H


///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL_Config.h"			 //
									 //
#include <vector>					 //
#include <sstream>					 //
#include <algorithm> 				 //
///////////////////////////////////////

namespace mysql
{
	class Impl;
	class Database
	{
	public:

		///////////////////////////////////////////////
		// Constructor                               //
		///////////////////////////////////////////////

		Database();

		void SetNewDatabase(std::shared_ptr<sql::Connection> NewConnection);

	private:
		std::shared_ptr<Impl> impl;

		///////////////////////////////////////////////
		// Private methods		                     //
		///////////////////////////////////////////////

	public:

		///////////////////////////////////////////////
		// Methods                                   //
		///////////////////////////////////////////////

		std::list<std::pair<std::string, nlohmann::json>> SelectValues(const std::string& name_table,
			const std::vector<std::string>& name_columns, const std::vector<std::string>& condition = {});

		void UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});
	};


	/////////////////////////////////////////////////////////////////////////////////
	// Definition of the most common names   	           	  	 				   //
	/////////////////////////////////////////////////////////////////////////////////

	typedef Database DATABASE;
	typedef Database MYSQL_DATABASE;

} // namespace mysql


#endif // MYSQL_DATABASE_H
