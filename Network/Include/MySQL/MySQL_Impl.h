#ifndef MYSQL_IMPL_H
#define MYSQL_IMPL_H


///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL_Config.h"			 //
									 //
#include <string>					 //
#include <numeric>					 //
									 //
///////////////////////////////////////


namespace mysql
{
	class Database;
	class Impl
	{
	public:
		Impl() {}
		Impl(std::shared_ptr<sql::Connection> NewConnection, std::shared_ptr<Database> NewDB = nullptr)
		{
			if (NewDB)
			{
				wasSelectedDB = true;
				current_database = NewDB;
			}

			connection = NewConnection;
		}
		~Impl() {}

		///////////////////////////////////////////////
		// Data type                                 //
		///////////////////////////////////////////////

		enum Status
		{
			Done,
			Error
		};

	private:
		sql::Driver *driver = nullptr;
		std::shared_ptr<sql::Connection> connection;
		static std::map<std::string, std::shared_ptr<mysql::Database>> databases;
		std::shared_ptr<Database> current_database;
		bool isReadOnly = false, wasSelectedDB = false;

		// For Reconnection
		static sql::ConnectOptionsMap connection_properties;
	public:
		//////////////////////////////////////////////////////////
		///////////			SECTION MODIFIED		//////////////
		//////////////////////////////////////////////////////////

		Status Connect(const std::string &user, const std::string &password, const std::string &host,
			const std::string &DB = "", const unsigned short &port = 3306, const std::string &charset = "utf8",
			bool OnlyRead = false);

		sql::ResultSet *Query(const std::string &query);
		void Exec(const std::string &query);

		void SelectDatabase(const std::string &name);

		std::shared_ptr<Database> GetCurrentDatabase() const;

		void TryInsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values, const std::vector<std::string> &condition = {});

		nlohmann::json TrySelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {});

		const std::vector<std::string> GetDatabaseNames() const;

		size_t GetSizeDatabases() const;

		//////////////////////////////////////////////////////////
		///////////		SECTION DON'T MODIFIED		//////////////
		//////////////////////////////////////////////////////////

		void CreateTable(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void CreateColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void ModifyColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values);

		void DeleteDatabase(const std::string &name);
		
		void CreateDatabase(const std::string &name);

		void AddDatabase(const std::string &name);

		void DeleteValues(const std::string &name_table, const std::string &condition = "");

		void DeleteTable(const std::string &name_table);

		void DeleteColumn(const std::string &name_table, const std::string &name_column);

		void Destroy();
	};


	/////////////////////////////////////////////////////////////////////////////////
	// Definition of the most common name   	           	  	 				   //
	/////////////////////////////////////////////////////////////////////////////////

	typedef Impl IMPL;

} // namespace mysql


#endif // MYSQL_IMPL_H
