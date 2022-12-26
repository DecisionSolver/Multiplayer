#include <pch.h>

#include <windows.h>

#include "ODBC/ODBC.h"
#include "DB_Query.hpp"
#include "boost/algorithm/string/case_conv.hpp"
#include <odbcinst.h>

#include <sql.h>
#include <sqlext.h>
#include <locale>
#include <codecvt>

std::string odbc::ODBC::DefaultDriverString = "Microsoft Access Driver (*.mdb)";

bool FindAnyNumberInString(const std::string &String)
{
	if (String.empty())
	{
		return false;
	}

	std::string::const_iterator it = String.begin();
	while (it != String.end() && std::isdigit(*it))
	{
		++it;
	}

	return it == String.end();
}

namespace odbc
{
	void ODBC::CreateDataBase(const std::string &driver, const std::string &path, const std::string &attributes,
		const std::string &password)
	{
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;

		if (!SQLConfigDataSourceW(nullptr, ODBC_ADD_DSN, converter.from_bytes(driver).c_str(),
			converter.from_bytes(("CREATE_DB=\"" + path + "\";" + attributes + (password.empty() ? "" : ";PWD=" + password))).c_str()))
		{
#if __has_include("logger.h")
			Logger_Error_F("Something is wrong with create a database file, error code: {}", GetLastError());
#endif
			DWORD error;
			WORD count;
			wchar_t MessageBuffer[1024];
			SQLInstallerErrorW(1, &error, MessageBuffer, sizeof(MessageBuffer), &count);

			std::wstringstream buf;
			buf << "Message: \"" << MessageBuffer << "\", Code: " << count;

			std::string ConvertedString = converter.to_bytes(buf.str());
			throw std::exception(ConvertedString.c_str());
		}
	}

	bool ODBC::Connect(const std::string &driver, const std::string &path, const std::string &Table,
		const std::vector<std::string> &attributes, const std::string &password)
	{
		if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &SQLHandleEnvironment) == SQL_ERROR)
		{
#if __has_include("logger.h")
			Logger_Error("Unable to allocate an environment handle\n");
#endif
			throw std::exception(("Something is wrong with Allocate Handle For SQL Environment, error code: " +
				std::to_string(GetLastError())).c_str());
		}

		if (PrintError(SQLHandleEnvironment, SQL_HANDLE_ENV, SQLSetEnvAttr(SQLHandleEnvironment, SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, 0)))
		{
			return false;
		}
		if (PrintError(SQLHandleEnvironment, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, SQLHandleEnvironment,
			&SQLHandleDatabaseConnection)))
		{
			return false;
		}

		std::string allattributes;
		for (auto attribute : attributes)
		{
			allattributes += attribute + ";";
		}

		if (PrintError(SQLHandleDatabaseConnection, SQL_HANDLE_DBC, SQLDriverConnectA(SQLHandleDatabaseConnection, nullptr,
			(SQLCHAR *)(("Driver={" + driver + "};Dbq=" + path + ";" + allattributes + "PWD=" + password + ";").c_str()),
			SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT)))
		{
			return false;
		}
		if (PrintError(SQLHandleDatabaseConnection, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT,
			SQLHandleDatabaseConnection, &SQLHandleStatement)))
		{
			return false;
		}

		SetCurrentTable(Table);
		
		return true;
	}
	bool ODBC::PrintError(SQLHANDLE Handle, SQLSMALLINT Type, SQLRETURN ErrorCode)
	{
		try
		{
			SQLSMALLINT iRec = 0;
			SQLINTEGER  iError = 0;
			SQLCHAR     wszMessage[1000], wszState[SQL_SQLSTATE_SIZE + 1];

			if (ErrorCode == SQL_INVALID_HANDLE)
			{
				throw std::exception("Invalid handle!");
			}

			while (SQLGetDiagRecA(Type, Handle, ++iRec, wszState, &iError, wszMessage, (SQLSMALLINT)(sizeof(wszMessage) /
				sizeof(CHAR)), 0) == SQL_SUCCESS)
			{
				std::stringstream MessageException;
				MessageException << "[" << wszState << "] " << wszMessage << " (" << iError << ")";
				throw std::exception(MessageException.str().c_str());
			}
		}
		catch (const std::exception &exception)
		{
#if __has_include("logger.h")
			Logger_Error(exception.what());
#endif
			if (std::string(exception.what()) == "Invalid handle!")
			{
				return true;
			}
		}
		return false;
	}
	int ODBC::GetCountRows(const std::string &query)
	{
		SQLHSTMT LocalDatabaseConnection = nullptr;
		if (PrintError(SQLHandleDatabaseConnection, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, SQLHandleDatabaseConnection,
			&LocalDatabaseConnection)))
		{
			return 0;
		}
		SQLExecDirectA(LocalDatabaseConnection, (SQLCHAR *)(query.c_str()), SQL_NTS);

		SQLLEN countRows = 0;
		while (SQLFetch(LocalDatabaseConnection) != SQL_NO_DATA)
		{
			countRows++;
		}

		PrintError(LocalDatabaseConnection, SQL_HANDLE_STMT, SQLFreeStmt(LocalDatabaseConnection, SQL_CLOSE));

		SQLFreeHandle(SQL_HANDLE_STMT, LocalDatabaseConnection);

		return countRows;
	}
	nlohmann::json ODBC::Query(const std::string &query, bool NeedDescribeColumnType)
	{
		RETCODE ErrorCode = SQLExecDirectA(SQLHandleStatement, (SQLCHAR *)(query.c_str()), SQL_NTS);
		nlohmann::json ReturnJSON = {};
		SQLLEN OldArraySize = 0, OldRowsetSize = 0;

		switch (ErrorCode)
		{
			case SQL_SUCCESS_WITH_INFO:
			{
				PrintError(SQLHandleStatement, SQL_HANDLE_STMT, ErrorCode);
			}
			case SQL_SUCCESS:
			{
				ErrorCode = SQLGetStmtAttr(SQLHandleStatement, SQL_ATTR_ROW_ARRAY_SIZE, &OldArraySize, sizeof(OldArraySize), NULL);
				ErrorCode = SQLGetStmtAttr(SQLHandleStatement, SQL_ROWSET_SIZE, &OldRowsetSize, sizeof(OldArraySize), NULL);
				ErrorCode = SQLSetStmtAttr(SQLHandleStatement, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)1, 0);
				ErrorCode = SQLSetStmtAttr(SQLHandleStatement, SQL_ROWSET_SIZE, (PTR)1, 0);

				SQLSMALLINT countColumns = 0;
				PrintError(SQLHandleStatement, SQL_HANDLE_STMT, SQLNumResultCols(SQLHandleStatement, &countColumns));
				std::vector<std::shared_ptr<ColunmStructure>> ResultSet;
				std::vector<std::pair<std::string, std::string>> columnNames = {};

				ErrorCode = SQLNumResultCols(SQLHandleStatement, &countColumns);
				if ((ErrorCode) != SQL_SUCCESS && (ErrorCode) != SQL_SUCCESS_WITH_INFO)
				{
					break;
				}
				for (SQLSMALLINT i = 0; i < countColumns; i++)
				{
					std::shared_ptr<ColunmStructure> Column = std::make_shared<ColunmStructure>();

					ErrorCode = SQLDescribeCol(SQLHandleStatement,
						(UWORD)(i + 1),
						(LPTSTR)Column->ColumnName,
						0, NULL,
						&Column->SqlType,
						&Column->MemorySize,
						NULL, NULL);

					if (ErrorCode != SQL_SUCCESS && ErrorCode != SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
					UINT sizeofWChar = sizeof(TCHAR);
					switch (Column->SqlType)
					{
						case SQL_BINARY:
						case SQL_VARBINARY:
						case SQL_LONGVARBINARY:
						{
							// Binary types must allow for twice as much room for the char display
							if (Column->MemorySize == 0)
							{
								//Handle MAX 
								Column->MemorySize = 8000;
							}
							else
							{
								Column->MemorySize *= (2 * sizeofWChar) + sizeofWChar;
								Column->SqlType = SQL_BINARY;
							}
							break;
						}
						case SQL_CHAR:
						case SQL_VARCHAR:
						case SQL_LONGVARCHAR:
						case SQL_WCHAR:
						case SQL_WVARCHAR:
						case SQL_WLONGVARCHAR:
						{
							Column->SqlType = SQL_CHAR;
							// Worst case, each Unicode char maps to a double-byte char
							// Prevent overflow if value is half a gig or larger
							if (Column->MemorySize < 0x7fffffff)
							{
								Column->MemorySize *= 2;
								Column->MemorySize += sizeofWChar;
							}
							else
							{
								Column->MemorySize = 0xffffffff;
							}
						}
						break;
						default:
						{
							// For other types, use a default buffer size
							Column->MemorySize = 100;
						}

					}
					Column->MemorySize = (Column->MemorySize < (UWORD)(-1) ? Column->MemorySize : (UWORD)(-1));
					Column->Pointer = (PTR)alloca(Column->MemorySize);

					char ColumnName[8192], ColumnType[255];
					PrintError(SQLHandleStatement, SQL_HANDLE_STMT, SQLColAttributeA(SQLHandleStatement, i + 1, SQL_COLUMN_NAME, ColumnName,
						sizeof(ColumnName), nullptr, nullptr));
					PrintError(SQLHandleStatement, SQL_HANDLE_STMT, SQLColAttributeA(SQLHandleStatement, i + 1, SQL_COLUMN_TYPE_NAME,
						ColumnType, sizeof(ColumnType), NULL, NULL));
					columnNames.push_back({ ColumnName, ColumnType });

					ResultSet.emplace_back(Column);
				}
				if (NeedDescribeColumnType)
				{
					for (size_t i = 0; i < columnNames.size(); i++)
					{
						ReturnJSON[columnNames[i].first].push_back({ { "sql_type", columnNames[i].second } });
					}
				}

				while ((ErrorCode = SQLFetch(SQLHandleStatement)) == SQL_STILL_EXECUTING);

				// Do A New SQLExecuteDirect To Get How Much Count Of Data Will Be
				SQLLEN countRows = 1;
				if (query.find("SELECT") != std::string::npos)
				{
					countRows = GetCountRows(query);
				}
				ErrorCode = SQL_SUCCESS;

				while ((ErrorCode) == SQL_SUCCESS || (ErrorCode) == SQL_SUCCESS_WITH_INFO)
				{
					for (size_t i = 0; i < ResultSet.size(); i++)
					{
						SQLLEN cbValue;
						PrintError(SQLHandleStatement,
							SQL_HANDLE_STMT, ErrorCode = SQLGetData(SQLHandleStatement,
								(UWORD)(i + 1),
								SQL_C_CHAR,
								ResultSet.at(i)->Pointer,
								ResultSet.at(i)->MemorySize,
								&cbValue));

						if ((ErrorCode == SQL_SUCCESS || (ErrorCode == SQL_SUCCESS_WITH_INFO && ErrorCode != SQL_NO_DATA_FOUND)) &&
							ResultSet.at(i)->Pointer != nullptr)
						{
							LPSTR PointerData = (LPSTR)ResultSet[i]->Pointer;

							if (cbValue == SQL_NULL_DATA)
							{
								if (countRows == 1 && countColumns == 1)
								{
									ReturnJSON = nlohmann::json();
								}
								else
								{
									ReturnJSON[columnNames[i].first].push_back(nlohmann::json()); // Was "NULL"
								}
								continue;
							}

							switch (ResultSet.at(i)->SqlType)
							{
								case SQL_BIT:
								case SQL_INTEGER:
								case SQL_NUMERIC:
								case SQL_TINYINT:
								case SQL_SMALLINT:
								case SQL_BIGINT:
								{
									if (countRows == 1 && countColumns == 1)
									{
										ReturnJSON = nlohmann::json::object({ { columnNames[i].first, atoi(PointerData) } });
									}
									else
									{
										ReturnJSON[columnNames[i].first].push_back(atoi(PointerData));
									}

									break;
								}
								case SQL_REAL:
								case SQL_DECIMAL:
								case SQL_DOUBLE:
								{
									if (countRows == 1 && countColumns == 1)
									{
										ReturnJSON = nlohmann::json::object({ { columnNames[i].first, atof(PointerData) } });
									}
									else
									{
										ReturnJSON[columnNames[i].first].push_back(atof(PointerData));
									}

									break;
								}
								case SQL_CHAR:
								case SQL_VARCHAR:
								case SQL_LONGVARCHAR:
								case SQL_WCHAR:
								case SQL_WVARCHAR:
								case SQL_WLONGVARCHAR:
								{
									nlohmann::json ParsedJSON;
									std::string StringFromPointerData = PointerData;
									
									if (!StringFromPointerData.empty())
									{
										if (StringFromPointerData.front() == '\"')
										{
											StringFromPointerData.erase(StringFromPointerData.begin());
										}
										if (StringFromPointerData.back() == '\"')
										{
											StringFromPointerData.erase(StringFromPointerData.end());
										}

										ParsedJSON = nlohmann::json::parse(StringFromPointerData, nullptr, false);

										if (ParsedJSON.is_discarded())
										{
											ParsedJSON = StringFromPointerData;
										}
									}
									if (countRows == 1 && countColumns == 1)
									{
										ReturnJSON = ParsedJSON;
									}
									else
									{
										ReturnJSON[columnNames[i].first].push_back(StringFromPointerData.empty() ? "" : ParsedJSON);
									}

									break;
								}
							}
						}
					}

					if ((ErrorCode != SQL_SUCCESS && ErrorCode != SQL_SUCCESS_WITH_INFO) && ErrorCode != SQL_NO_DATA_FOUND)
					{
						break;
					}
					while ((ErrorCode = SQLFetch(SQLHandleStatement)) == SQL_STILL_EXECUTING);
				}
				break;
			}
			case SQL_ERROR:
			{
				PrintError(SQLHandleStatement, SQL_HANDLE_STMT, ErrorCode);
				break;
			}

			// In Somecase it may consider like success!
			case SQL_NO_DATA_FOUND:
			{
				break;
			}

			default:
			{
#if __has_include("logger.h")
				Logger_Error_F("Unexpected return code {}!\n", ErrorCode);
#endif
				break;
			}
		}

		PrintError(SQLHandleStatement, SQL_HANDLE_STMT, SQLFreeStmt(SQLHandleStatement, SQL_CLOSE));

		//Reset rowset sizes
		SQLSetStmtAttr(SQLHandleStatement, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)(LONG_PTR)OldArraySize, sizeof(OldArraySize));
		SQLSetStmtAttr(SQLHandleStatement, SQL_ROWSET_SIZE, (PTR)(LONG_PTR)OldRowsetSize, sizeof(OldArraySize));

		return ReturnJSON;
	}

	nlohmann::json ODBC::SelectValues(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &condition, bool NeedDescribeColumnType)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
			return nlohmann::json();
		}
		return Query(query::MakeSelectValuesQuery(name_table, name_columns, condition), NeedDescribeColumnType);
	}

	nlohmann::json ODBC::SelectValuesInCurrentTable(const std::vector<std::string> &name_columns,
		const std::vector<std::string> &condition, bool NeedDescribeColumnType)
	{
		return Query(query::MakeSelectValuesQuery(CurrentTable,	name_columns, condition), NeedDescribeColumnType);
	}

	void ODBC::InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}
		Query(query::MakeInsertValuesQuery(name_table, name_columns, values));
	}

	void ODBC::InsertValuesInCurrentTable(const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values)
	{
		Query(query::MakeInsertValuesQuery(CurrentTable, name_columns, values));
	}

	void ODBC::UpdateValues(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values, const std::vector<std::string> &condition)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Query(query::MakeUpdateValuesQuery(name_table, name_columns, values, condition));
	}

	void ODBC::UpdateValuesInCurrentTable(const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values, const std::vector<std::string> &condition)
	{
		Query(query::MakeUpdateValuesQuery(CurrentTable, name_columns, values, condition));
	}

	void ODBC::CreateAndSetCurrentTable(const std::string &name_table, const std::vector<std::string> &name_column,
		const std::vector<std::string> &type, const std::vector<std::string> &value,
		const std::vector<std::vector<std::string>> &attributes)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		std::string result_query = query::MakeCreateTableQuery(name_table, name_column, type, value, attributes);

		if (result_query.empty())
		{
			throw std::exception("Query Was Build With Errors!");
		}

		if (result_query.find("DEFAULT CHARSET UTF8") != std::string::npos)
		{
			result_query.erase(result_query.find("DEFAULT CHARSET UTF8"), 20);
		}

		Query(result_query);

		CurrentTable = name_table;
	}
	void ODBC::CreateTable(const std::string &name_table, const std::vector<std::string> &name_column,
		const std::vector<std::string> &type, const std::vector<std::string> &value,
		const std::vector<std::vector<std::string>> &attributes)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		std::string TmpCurrentTable = CurrentTable;

		CreateAndSetCurrentTable(name_table, name_column, type, value, attributes);

		CurrentTable = TmpCurrentTable;
	}

	void ODBC::CreateColumn(const std::string &name_table, const std::string &name_column, const std::string &type,
		const std::string &value, const std::vector<std::string> &attributes)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Query(query::MakeCreateColumnQuery(name_table, name_column, type, value, attributes));
	}

	void ODBC::CreateColumnInCurrentTable(const std::string &name_column, const std::string &type,
		const std::string &value, const std::vector<std::string> &attributes)
	{
		Query(query::MakeCreateColumnQuery(CurrentTable, name_column, type, value, attributes));
	}

	void ODBC::ModifyColumn(const std::string &name_table, const std::string &name_column, const std::string &type,
		const std::string &value, const std::vector<std::string> &attributes)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		std::string QueryResult = query::MakeModifyColumnQuery(name_table, name_column, type, value, attributes);

		if (QueryResult.empty())
		{
			throw std::exception("Query Was Build With Errors!");
		}

		size_t FindedPosition = QueryResult.find("MODIFY");
		if (FindedPosition != std::string::npos)
		{
			QueryResult.erase(FindedPosition, 6);
			QueryResult.insert(FindedPosition, "ALTER");
		}

		Query(QueryResult);
	}

	void ODBC::ModifyColumnInCurrentTable(const std::string &name_column, const std::string &type,
		const std::string &value, const std::vector<std::string> &attributes)
	{
		ModifyColumn(CurrentTable, name_column, type, value, attributes);
	}

	void ODBC::DeleteTable(const std::string &name_table)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Query(query::MakeDeleteTableQuery(name_table));
	}

	void ODBC::DeleteCurrentTable()
	{
		Query(query::MakeDeleteTableQuery(CurrentTable));
	}

	void ODBC::DeleteColumn(const std::string &name_table, const std::string &name_column)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Query(query::MakeDeleteColumnQuery(name_table, name_column));
	}

	void ODBC::DeleteColumnInCurrentTable(const std::string &name_column)
	{
		Query(query::MakeDeleteColumnQuery(CurrentTable, name_column));
	}

	void ODBC::DeleteValues(const std::string &name_table, const std::string &condition)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Query(query::MakeDeleteValuesQuery(name_table, condition));
	}

	void ODBC::DeleteValuesInCurrentTable(const std::string &condition)
	{
		Query(query::MakeDeleteValuesQuery(CurrentTable, condition));
	}

	void ODBC::Destroy()
	{
		if (SQLHandleStatement)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, SQLHandleStatement);
		}

		if (SQLHandleDatabaseConnection)
		{
			SQLDisconnect(SQLHandleDatabaseConnection);
			SQLFreeHandle(SQL_HANDLE_DBC, SQLHandleDatabaseConnection);
		}

		if (SQLHandleEnvironment)
		{
			SQLFreeHandle(SQL_HANDLE_ENV, SQLHandleEnvironment);
		}
	}

	std::pair<bool, std::vector<std::string>> ODBC::GetListTablesDatabase()
	{
		std::pair<bool, std::vector<std::string>> ReturnData = { false, {} };
		std::vector<std::shared_ptr<ColunmStructure>> ResultSet;
		SQLSMALLINT SizeColumns;
		SQLRETURN ErrorCode = SQL_SUCCESS;
		// Invoke function
		SQLLEN OldArraySize = 0, OldRowsetSize = 0;

		ErrorCode = SQLGetStmtAttr(SQLHandleStatement, SQL_ATTR_ROW_ARRAY_SIZE, &OldArraySize, sizeof(OldArraySize), NULL);
		ErrorCode = SQLGetStmtAttr(SQLHandleStatement, SQL_ROWSET_SIZE, &OldRowsetSize, sizeof(OldArraySize), NULL);
		ErrorCode = SQLSetStmtAttr(SQLHandleStatement, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)1, 0);
		ErrorCode = SQLSetStmtAttr(SQLHandleStatement, SQL_ROWSET_SIZE, (PTR)1, 0);

		ErrorCode = SQLTables(SQLHandleStatement,
			SQL_NULL_HANDLE, // szTableQualifier
			0,				// cbTableQualifier
			SQL_NULL_HANDLE, // szTableOwner
			SQL_NULL_HANDLE, // cbTableOwner
			0,				// szTableName
			SQL_NULL_HANDLE, // cbTableName
			0,				// szTableType
			SQL_NULL_HANDLE); // cbTableType

		ErrorCode = SQLNumResultCols(SQLHandleStatement, &SizeColumns);
		if ((ErrorCode) != SQL_SUCCESS && (ErrorCode) != SQL_SUCCESS_WITH_INFO)
		{
			return ReturnData;
		}

		for (SQLSMALLINT i = 0; i < SizeColumns; i++)
		{
			std::shared_ptr<ColunmStructure> Column = std::make_shared<ColunmStructure>();

			ErrorCode = SQLDescribeCol(SQLHandleStatement,
				(UWORD)(i + 1),
				(LPTSTR)Column->ColumnName,
				0, NULL,
				&Column->SqlType,
				&Column->MemorySize,
				NULL, NULL);

			if (ErrorCode != SQL_SUCCESS && ErrorCode != SQL_SUCCESS_WITH_INFO)
			{
				return ReturnData;
			}

			UINT sizeofChar = sizeof(TCHAR);
			switch (Column->SqlType)
			{
				case SQL_BINARY:
				case SQL_VARBINARY:
				case SQL_LONGVARBINARY:
				{
					// Binary types must allow for twice as much room for the char display
					if (Column->MemorySize == 0)
					{
						//Handle MAX 
						Column->MemorySize = 8000;
					}
					else
					{
						Column->MemorySize *= (2 * sizeofChar) + sizeofChar;
						Column->SqlType = SQL_BINARY;
					}
				
					break;
				}
				case SQL_CHAR:
				case SQL_VARCHAR:
				case SQL_LONGVARCHAR:
				case SQL_WCHAR:
				case SQL_WVARCHAR:
				case SQL_WLONGVARCHAR:
				{
					Column->SqlType = SQL_CHAR;
					// Worst case, each Unicode char maps to a double-byte char
					// Prevent overflow if value is half a gig or larger
					if (Column->MemorySize < 0x7fffffff)
					{
						Column->MemorySize *= 2;
						Column->MemorySize += sizeofChar;
					}
					else
					{
						Column->MemorySize = 0xffffffff;
					}

					break;
				}
				default:
				{
					// For other types, use a default buffer size
					Column->MemorySize = 100;
				}
			}

			Column->MemorySize = (Column->MemorySize < (UWORD)(-1) ? Column->MemorySize : (UWORD)(-1));
			Column->Pointer = (PTR)alloca(Column->MemorySize);

			ResultSet.emplace_back(Column);
		}

		while ((ErrorCode = SQLFetch(SQLHandleStatement)) == SQL_STILL_EXECUTING);

		ErrorCode = SQL_SUCCESS;

		while ((ErrorCode) == SQL_SUCCESS || (ErrorCode) == SQL_SUCCESS_WITH_INFO)
		{
			for (size_t i = 1; i < ResultSet.size(); i++)
			{
				SQLLEN cbValue;
				PrintError(SQLHandleStatement,
					SQL_HANDLE_STMT, ErrorCode = SQLGetData(SQLHandleStatement,
						(UWORD)(i + 1),
						SQL_C_CHAR,
						ResultSet.at(i)->Pointer,
						ResultSet.at(i)->MemorySize,
						&cbValue));

				if (cbValue < 0)
				{
					continue;
				}

				if ((ErrorCode == SQL_SUCCESS || (ErrorCode == SQL_SUCCESS_WITH_INFO && ErrorCode != SQL_NO_DATA_FOUND)) &&
					ResultSet.at(i)->Pointer != nullptr)
				{
					std::string TypeColumn = (LPSTR)ResultSet.at(i)->Pointer;
					ReturnData.second.push_back(TypeColumn);
					if (TypeColumn == "SYSTEM TABLE" || TypeColumn == "TABLE")
					{
						if (TypeColumn != "TABLE")
						{
							for (size_t _ = 0; _ < 2; _++)
							{
								if (!ReturnData.second.empty())
								{
									ReturnData.second.pop_back();
								}
							}
						}
						else
						{
							auto Iter = ReturnData.second.begin();
							while (Iter < ReturnData.second.end())
							{
								if (*Iter == "TABLE")
								{
									Iter = ReturnData.second.erase(Iter);
									ReturnData.first = true;
								}
								else
								{
									Iter++;
								}
							}
						}
					}
				}
			}

			if ((ErrorCode != SQL_SUCCESS && ErrorCode != SQL_SUCCESS_WITH_INFO) && ErrorCode != SQL_NO_DATA_FOUND)
			{
				break;
			}

			while ((ErrorCode = SQLFetch(SQLHandleStatement)) == SQL_STILL_EXECUTING);
		}

		SQLFreeStmt(SQLHandleStatement, SQL_CLOSE);

		//Reset rowset sizes
		SQLSetStmtAttr(SQLHandleStatement, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)(LONG_PTR)OldArraySize, sizeof(OldArraySize));
		SQLSetStmtAttr(SQLHandleStatement, SQL_ROWSET_SIZE, (PTR)(LONG_PTR)OldRowsetSize, sizeof(OldArraySize));

		return ReturnData;
	}

	void ODBC::SplitDB(const std::string &NameTable, const std::string &NameNewFile)
	{
		if (!SQLHandleEnvironment || !SQLHandleDatabaseConnection || !SQLHandleStatement)
		{
#if __has_include("logger.h")
			Logger_Error("This Function Requires To Be Connected To DB! Aborting!");
#endif
			return;
		}

		if (NameTable.empty() || NameNewFile.empty())
		{
#if __has_include("logger.h")
			Logger_Error("This Function Requires Params To Work! Aborting!");
#endif
			return;
		}

		auto SecondFile = std::make_shared<ODBC>();
		// Create A New DB
		SecondFile->CreateDataBase(DefaultDriverString, NameNewFile);
		SecondFile->Connect(DefaultDriverString, NameNewFile, "", {});

		std::string table = !NameTable.empty() ? NameTable : CurrentTable;
		// Then Get * From NameTable And Add It To New DB
		auto RowsFromDatabase = SelectValues(table, { "*" }, {}, true);

		std::vector<std::string> NameColumns, TypeColumns, EmptyValue;
		std::vector<std::vector<std::string>> EmptyAttributes;
		for (auto it = RowsFromDatabase.begin(); it != RowsFromDatabase.end(); ++it)
		{
			NameColumns.push_back(it.key());

			if (!it.value().front()["sql_type"].is_null() || !it.value().front()["sql_type"].empty())
			{
				TypeColumns.push_back(it.value().front()["sql_type"]);
			}

			EmptyValue.push_back({});
			EmptyAttributes.push_back({});
		}
		SecondFile->CreateAndSetCurrentTable(table, NameColumns, TypeColumns, EmptyValue, EmptyAttributes);

		size_t sizeRows = 
			(size_t)Query("SELECT COUNT(*) FROM `" + table + "`").front().back().get<nlohmann::json::number_integer_t>(),
			currentRowIndex = 1u;
		
		while (true)
		{
			std::vector<std::string> columns, value;
			for (size_t i = 0; i < NameColumns.size(); i++)
			{
				columns.push_back(NameColumns.at(i));

				auto It = RowsFromDatabase[NameColumns.at(i)];
				if (It.is_string() || It.is_object())
				{
					value.push_back(It);
				}
				else if (It.is_array())
				{
					value.push_back(It.at(currentRowIndex));
				}
			}

			SecondFile->InsertValues(NameTable, columns, value);

			if (currentRowIndex >= sizeRows)
			{
				break;
			}
			else
			{
				currentRowIndex++;
			}
		}
		SecondFile->Destroy();
	}
} // namespace odbc
