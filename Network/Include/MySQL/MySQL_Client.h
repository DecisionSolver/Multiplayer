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
		mutable sql::Driver *driver = nullptr;
		mutable std::shared_ptr<sql::Connection> connection;
		bool isReadOnly = false;
		std::string CurrentDB;
		std::string CurrentTable;
		// For Reconnection
		static sql::ConnectOptionsMap connection_properties;
	public:
		enum Status
		{
			Done,
			Error
		};

		const std::string GetCurrentDB() const { return CurrentDB; }
		const std::string GetCurrentTable() const { return CurrentTable; }

		void SetCurrentDB(const std::string &NameDB)
		{
			if (!NameDB.empty())
				CurrentDB = NameDB;
		}
		void SetCurrentTable(const std::string &TableName) { CurrentTable = TableName; }

		//////////////////////////////////////////////////////////
		///////////			SECTION MODIFIED		//////////////
		//////////////////////////////////////////////////////////

		Status Connect(const std::string &user, const std::string &password, const std::string &host,
			const std::string &DB, const std::string &Table, const unsigned short &port = 3306,
			const std::string &charset = "utf8", bool OnlyRead = false);

		sql::ResultSet *Query(const std::string &query) const;
		void Exec(const std::string &query) const;

		void InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values) const;

		nlohmann::json SelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {}) const;

		void CreateTable(const std::string &name_table, const std::vector<std::string> &name_column,
			const std::vector<std::string> &type, const std::vector<std::string> &value,
			const std::vector<std::vector<std::string>> &attributes) const;

		void CreateColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes) const;

		void ModifyColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes) const;

		void DeleteDatabase(const std::string &name) const;

		void CreateDatabase(const std::string &name) const;

		void DeleteValues(const std::string &name_table, const std::string &condition = "") const;

		void DeleteTable(const std::string &name_table) const;

		void DeleteColumn(const std::string &name_table, const std::string &name_column) const;

		void Destroy() const;

		void UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {}) const;
	};


	/////////////////////////////////////////////////////////////////////////////////
	// Definition of the most common names   	           	  	 				   //
	/////////////////////////////////////////////////////////////////////////////////

	typedef Client CLIENT;
	typedef Client MYSQLCLIENT;

} // namespace mysql


#endif // MYSQL_CLIENT_H
