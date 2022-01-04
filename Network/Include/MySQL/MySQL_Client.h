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
		const std::string CurrentDB;
		const std::string CurrentTable;
		// For Reconnection
		static sql::ConnectOptionsMap connection_properties;
	public:
		enum Status
		{
			Done,
			Error
		};

		const std::string GetCurrentDB() { return CurrentDB; }
		const std::string GetCurrentTable() { return CurrentTable; }

		void SetCurrentDB(const std::string &NameDB)
		{
			if (!NameDB.empty())
				const_cast<std::string &>(CurrentDB) = NameDB;
		}
		void SetCurrentTable(const std::string &TableName)
		{
			if (!TableName.empty())
				const_cast<std::string &>(CurrentTable) = TableName;
		}

		//////////////////////////////////////////////////////////
		///////////			SECTION MODIFIED		//////////////
		//////////////////////////////////////////////////////////

		Status Connect(const std::string &user, const std::string &password, const std::string &host,
			const std::string &DB, const std::string &Table, const unsigned short &port = 3306,
			const std::string &charset = "utf8", bool OnlyRead = false);

		sql::ResultSet *Query(const std::string &query);
		void Exec(const std::string &query);

		void InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values);

		// Into Current Table
		void InsertValues(const std::vector<std::string> &name_columns, const std::vector<std::string> &values);

		nlohmann::json SelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {});

		// From Current Table
		nlohmann::json SelectValues(const std::vector<std::string> &name_columns,
			const std::vector<std::string> &condition = {});

		void CreateTable(const std::string &name_table, const std::vector<std::string> &name_column,
			const std::vector<std::string> &type, const std::vector<std::string> &value,
			const std::vector<std::vector<std::string>> &attributes);

		void CreateColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		// Used Current Table
		void CreateColumn(const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void ModifyColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		// Used Current Table
		void ModifyColumn(const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void DeleteDatabase(const std::string &name);

		void CreateDatabase(const std::string &name);

		void DeleteValues(const std::string &name_table, const std::string &condition = "");

		// From Current Table
		void DeleteValues(const std::string &condition = "");

		void DeleteTable(const std::string &name_table);

		// Used Current Table
		void DeleteTable();

		void DeleteColumn(const std::string &name_table, const std::string &name_column);

		// Used Current Table
		void DeleteColumn(const std::string &name_column);

		void Destroy();

		void UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});

		// Used Current Table
		void UpdateValues(const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});
	};


	/////////////////////////////////////////////////////////////////////////////////
	// Definition of the most common names   	           	  	 				   //
	/////////////////////////////////////////////////////////////////////////////////

	typedef Client CLIENT;
	typedef Client MYSQLCLIENT;

} // namespace mysql


#endif // MYSQL_CLIENT_H
