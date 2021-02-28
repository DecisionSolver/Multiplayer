#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H


///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL_Config.h"			 //	
									 //
#include <map>						 //
#include <vector>					 //
#include <sstream>					 //
									 //
///////////////////////////////////////


namespace mysql
{
	class Client
	{
	private:
		sql::Driver *driver = nullptr;
		std::shared_ptr<sql::Connection> connection;
		bool isReadOnly = false, wasSelectedDB = false;

		// For Reconnection
		static sql::ConnectOptionsMap connection_properties;
	public:
		enum Status
		{
			Done,
			Error
		};

		//////////////////////////////////////////////////////////
		///////////			SECTION MODIFIED		//////////////
		//////////////////////////////////////////////////////////

		Status Connect(const std::string &user, const std::string &password, const std::string &host,
			const std::string &DB = {}, const unsigned short &port = 3306, const std::string &charset = "utf8",
			bool OnlyRead = false);

		sql::ResultSet *Query(const std::string &query);
		void Exec(const std::string &query);

		void TryInsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values, const std::vector<std::string> &condition = {});

		nlohmann::json TrySelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {});

		void CreateTable(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void CreateColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void ModifyColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void DeleteDatabase(const std::string &name);

		void CreateDatabase(const std::string &name);

		void DeleteValues(const std::string &name_table, const std::string &condition = "");

		void DeleteTable(const std::string &name_table);

		void DeleteColumn(const std::string &name_table, const std::string &name_column);

		void Destroy();

		void Disconnect();

		void TryUpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});
	};


	/////////////////////////////////////////////////////////////////////////////////
	// Definition of the most common names   	           	  	 				   //
	/////////////////////////////////////////////////////////////////////////////////

	typedef Client CLIENT;
	typedef Client MYSQLCLIENT;

} // namespace mysql


#endif // MYSQL_CLIENT_H
