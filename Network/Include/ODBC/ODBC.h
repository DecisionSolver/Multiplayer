#ifndef ODBC_H
#define ODBC_H

#include <winapifamily.h>
#include <pch.h>

typedef void *SQLHENV;
typedef void *SQLHDBC;
typedef void *SQLHSTMT;
typedef void *SQLHANDLE;
typedef short SQLSMALLINT;
typedef SQLSMALLINT SQLRETURN;
namespace odbc
{
	class ODBC
	{
	private:
		SQLHENV SQLHandleEnvironment = nullptr;
		SQLHDBC SQLHandleDatabaseConnection = nullptr;
		SQLHSTMT SQLHandleStatement = nullptr;

		std::string CurrentTable;

		bool PrintError(SQLHANDLE Handle, SQLSMALLINT Type, SQLRETURN ErrorCode);
		struct ColunmStructure
		{
			const wchar_t* ColumnName[255]; // Column name for display
			short SqlType; // For GetData call
			unsigned long MemorySize; // How much memory to allocate
			void* Pointer; // Pointer to memory
		};

	public:
		ODBC() = default;
		~ODBC() = default;

		std::string GetCurrentTable() const { return CurrentTable; }
		void SetCurrentTable(const std::string &TableName)
		{
			if (!TableName.empty())
				CurrentTable = TableName;
		}

		void CreateDataBase(const std::string &driver, const std::string &path,
			const std::string &attributes = {}, const std::string &password = {});

		bool Connect(const std::string &driver, const std::string &path, const std::string &Table,
			const std::vector<std::string> &attributes, const std::string &password = {});

		nlohmann::json SelectValues(const std::string &name_table,
			const std::vector<std::string> &name_columns, const std::vector<std::string> &condition = {},
			bool NeedDescribeColumnType = false);

		nlohmann::json SelectValuesInCurrentTable(const std::vector<std::string> &name_columns,
			const std::vector<std::string> &condition = {}, bool NeedDescribeColumnType = false);

		void InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values);

		void InsertValuesInCurrentTable(const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values);

		void UpdateValues(const std::string &name_table, const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values, const std::vector<std::string> &condition = {});

		void UpdateValuesInCurrentTable(const std::vector<std::string> &name_columns,
			const std::vector<std::string> &values, const std::vector<std::string> &condition = {});

		void CreateTable(const std::string &name_table, const std::vector<std::string> &name_column,
			const std::vector<std::string> &type, const std::vector<std::string> &value,
			const std::vector<std::vector<std::string>> &attributes);

		void CreateAndSetCurrentTable(const std::string &name_table, const std::vector<std::string> &name_column,
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

		void DeleteTable(const std::string &name_table);

		void DeleteCurrentTable();

		void DeleteColumn(const std::string &name_table, const std::string &name_column);
		
		void DeleteColumnInCurrentTable(const std::string &name_column);

		void DeleteValues(const std::string &name_table, const std::string &condition);
		
		void DeleteValuesInCurrentTable(const std::string &condition);
		
		// For Other Stuffs
		nlohmann::json Query(const std::string& query, bool NeedDescribeColumnType = false);

		void Destroy();

		// Returns false if error was occured or DB is empty (has no one tables), else true if tables exist
		std::pair<bool, std::vector<std::string>> GetListTablesDatabase();

		// Splits Table Into One DB File
		void SplitDB(const std::string &NameTable, const std::string &NameNewFile);

		int GetCountRows(const std::string &query);

		static std::string DefaultDriverString;
	};
} // namespace odbc

#endif //ODBC_H
