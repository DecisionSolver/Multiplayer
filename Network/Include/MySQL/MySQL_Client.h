#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H

#include "MySQL_Config.h"

#include <map>
#include <vector>
#include <sstream>

namespace mysql
{
	class Client
	{
	private:
		bool TryException(const std::function<void()> &functionToCatch, const std::string &UserData = {});

		static std::shared_ptr<sql::Connection> connection;
		bool isReadOnly = false;
		std::string CurrentDatabase;
		std::string CurrentTable;

		// For Reconnection
		static sql::ConnectOptionsMap connection_properties;
		
		std::condition_variable IsReadyToGo;

		void Destroy();
	public:
		~Client()
		{
			Destroy();
		}
		enum Status
		{
			Done,
			Error
		};

		const std::string GetCurrentDatabase() const { return CurrentDatabase; }
		const std::string GetCurrentTable() const { return CurrentTable; }

		void SetCurrentDatabase(const std::string &DataBase);
		void SetCurrentTable(const std::string &TableName);

		Status Connect(const std::string &user, const std::string &password, const std::string &host,
			const std::string &Database = {}, const std::string &Table = {}, const unsigned short &port = 3306,
			const std::string &charset = "utf8", bool ReadOnly = false);

		sql::ResultSet *Query(const std::string &query);
		void Exec(const std::string &query);

		void InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values);

		void InsertValuesInCurrentTable(const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values);

		nlohmann::json SelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {});

		nlohmann::json SelectValuesInCurrentTable(const std::vector<std::string> &name_columns,
			const std::vector<std::string> &condition = {});

		void CreateTable(const std::string &name_table, const std::vector<std::string> &name_column,
			const std::vector<std::string> &type, const std::vector<std::string> &value,
			const std::vector<std::vector<std::string>> &attributes);

		void CreateColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void CreateColumnInCurrentTable(const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void ModifyColumn(const std::string &name_table, const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void ModifyColumnInCurrentTable(const std::string &name_column,
			const std::string &type, const std::string &value, const std::vector<std::string> &attributes);

		void DeleteDatabase(const std::string &name_database);

		void CreateDatabaseAndSetCurrent(const std::string &name_database);

		void DeleteValues(const std::string &name_table, const std::string &condition = {});

		void DeleteValuesInCurrentTable(const std::string &condition = {});

		void DeleteTable(const std::string &name_table);

		void DeleteCurrentTable();

		void DeleteColumn(const std::string &name_table, const std::string &name_column);

		void DeleteColumnInCurrentTable(const std::string &name_column);

		void UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});

		void UpdateValuesInCurrentTable(const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});
	};
}
#endif