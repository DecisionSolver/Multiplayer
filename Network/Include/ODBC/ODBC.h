#ifndef ODBC_H
#define ODBC_H

#include <winapifamily.h>

#include <pch.h>

typedef void* SQLHENV;
typedef void* SQLHDBC;
typedef void* SQLHSTMT;
typedef void* SQLHANDLE;
namespace odbc
{
	class ODBC
	{
	private:
		SQLHENV  hEnv  = NULL;
		SQLHDBC  hDbc  = NULL;
		SQLHSTMT hStmt = NULL;

		std::string CurrentTable;

		bool PrintError(SQLHANDLE hHandle, short hType, short e) const;
		typedef struct tagGETINFOALL {
			const wchar_t*				szCol[255];			// Column name for display
			short				fSqlType;						// For GetData call
			unsigned long		cbValueMax;						// How much memory to allocate
			void*				rgbValue;						// Pointer to memory
		} GETINFOALL;
		typedef GETINFOALL * lpGETINFOALL;

		int GetCntData(const std::string &query) const;

	public:
		ODBC() {}
		~ODBC() {}

		const std::string GetCurrentTable() const { return CurrentTable; }
		void SetCurrentTable(const std::string& TableName) { CurrentTable = TableName; }

		void CreateDataBase(const std::string& driver, const std::string& path,
			const std::string& attributes = {}, const std::string& password = {}) const;
		bool Connect(const std::string& driver, const std::string& path, const std::string& Table,
			const std::vector<std::string>& attributes, const std::string& password = {});

		nlohmann::json SelectValues(const std::string& name_table,
			const std::vector<std::string>& name_columns, const std::vector<std::string>& condition = {},
			bool Need_SQL_TYPE = false) const;
		void InsertValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values) const;
		void UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
			const std::vector<std::string>& values, const std::vector<std::string>& condition = {}) const;

		void CreateTable(const std::string& name_table, const std::vector<std::string>& name_column,
			const std::vector<std::string>& type, const std::vector<std::string>& value, 
			const std::vector<std::vector<std::string>>& attributes) const;
		void CreateColumn(const std::string& name_table, const std::string& name_column,
			const std::string& type, const std::string& value, const std::vector<std::string>& attributes) const;
		void ModifyColumn(const std::string& name_table, const std::string& name_column,
			const std::string& type, const std::string& value, const std::vector<std::string>& attributes) const;

		void DeleteTable(const std::string& name_table) const;
		void DeleteColumn(const std::string& name_table, const std::string& name_column) const;
		void DeleteValues(const std::string& name_table, const std::string& condition) const;
		
		// For Other Stuffs
		nlohmann::json Query(const std::string& query, bool Need_SQL_TYPE = false) const;

		void Exit() const;

		// Returns false if error was occured or DB (has no one tables) is empty, else true if tables exist
		std::pair<bool, std::vector<std::string>> GetListTablesDatabase() const;

		// Splits Table Into One DB File
		void SplitDB(const std::string &NameTable, const std::string &NameNewFile) const;
	};
} // namespace odbc

#endif //ODBC_H
