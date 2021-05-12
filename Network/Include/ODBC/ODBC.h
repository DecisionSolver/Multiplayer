#ifndef ODBC_H
#define ODBC_H

#include <pch.h>
#include <sql.h>
#include <sqlext.h>

namespace odbc
{
	class ODBC
	{
	private:
		SQLHENV  hEnv  = NULL;
		SQLHDBC  hDbc  = NULL;
		SQLHSTMT hStmt = NULL;

		nlohmann::json Query(const std::string& query);
		bool PrintError(SQLHANDLE hHandle, SQLSMALLINT hType, SQLRETURN e);

	public:
		ODBC() {}
		~ODBC() {}
		void CreateDataBase(const std::string& driver, const std::string& path,
			const std::string& attributes = {}, const std::string& password = {});
		void Connect(const std::string& driver, const std::string& path,
			const std::string& attributes, const std::string& password);
		nlohmann::json SelectValues(const std::string& name_table,
			const std::vector<std::string>& name_columns, const std::vector<std::string>& condition = {});
		void InsertValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values);
		void UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {});
		void CreateTable(const std::string& name_table, const std::string& name_column,
			const std::string& type, const std::string& value, const std::vector<std::string>& attributes);
		void CreateColumn(const std::string& name_table, const std::string& name_column,
			const std::string& type, const std::string& value, const std::vector<std::string>& attributes);
		void ModifyColumn(const std::string& name_table, const std::string& name_column,
			const std::string& type, const std::string& value, const std::vector<std::string>& attributes);
		void DeleteTable(const std::string& name_table);
		void DeleteColumn(const std::string& name_table, const std::string& name_column);
		void DeleteValues(const std::string& name_table, const std::string& condition);
		void Exit();
	};
} // namespace odbc

#endif //ODBC_H
