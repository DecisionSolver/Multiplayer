#include <pch.h>
#include "ODBC/ODBC.h"
#include "SQL_Query.hpp"

namespace odbc
{
	void ODBC::Connect(const std::string& driver, const std::string& path, const std::string& attributes, const std::string& password)
	{
		if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
		{
			std::cerr << "Unable to allocate an environment handle\n";
			return;
		}

		if (PrintError(hEnv, SQL_HANDLE_ENV, SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)))
			return;

		if (PrintError(hEnv, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)))
			return;

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)(("Driver={" + driver + "};Dbq=" + path + ";" + attributes + ";" + password + ";").c_str()), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT)))
			return;

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)))
			return;
	}

	bool ODBC::PrintError(SQLHANDLE hHandle, SQLSMALLINT hType, SQLRETURN e)
	{
		SQLSMALLINT iRec = 0;
		SQLINTEGER  iError;
		SQLCHAR       wszMessage[1000];
		SQLCHAR       wszState[SQL_SQLSTATE_SIZE + 1];

		if (e == SQL_INVALID_HANDLE)
		{
			fwprintf(stderr, L"Invalid handle!\n");
			return false;
		}

		if (e == SQL_ERROR)
			std::cerr << "Critical error: ";

		while (SQLGetDiagRecA(hType, hHandle, ++iRec, wszState, &iError, wszMessage, (SQLSMALLINT)(sizeof(wszMessage) / sizeof(CHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS)
		{
			// Hide data truncated..
			if (strncmp((const char*)wszState, "01004", 5))
			{
				fprintf(stderr, "[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
			}
		}

		return false;
	}

	nlohmann::json ODBC::Query(const std::string& query)
	{
		RETCODE RetCode = SQLExecDirectA(hStmt, (SQLCHAR*)(query.c_str()), SQL_NTS);
		json res = {};

		switch (RetCode)
		{

		case SQL_SUCCESS_WITH_INFO:
		{
			PrintError(hStmt, SQL_HANDLE_STMT, RetCode);
			// fall through
		}

		case SQL_SUCCESS:
		{
			SQLSMALLINT sNumResults;
			PrintError(hStmt, SQL_HANDLE_STMT, SQLNumResultCols(hStmt, &sNumResults));

			if (sNumResults > 0)
			{
				while (SQLFetch(hStmt) == SQL_SUCCESS)
				{
					for (int i = 1; i <= sNumResults; i++)
					{
						SQLINTEGER indicator;
						char buf[512];

						if (SQLGetData(hStmt, i, SQL_C_CHAR, buf, sizeof(buf), &indicator) == SQL_SUCCESS)
						{
							if (indicator == SQL_NULL_DATA)
								strcpy(buf, "NULL");

							res[i].push_back(buf);
						}
					}
				}
			}

			break;
		}

		case SQL_ERROR:
		{
			PrintError(hStmt, SQL_HANDLE_STMT, RetCode);
			break;
		}

		default:
			fwprintf(stderr, L"Unexpected return code %hd!\n", RetCode);
		}

		PrintError(hStmt, SQL_HANDLE_STMT, SQLFreeStmt(hStmt, SQL_CLOSE));

		return res;
	}

	nlohmann::json ODBC::SelectValues(const std::string& name_table, const std::vector<std::string>& name_columns, const std::vector<std::string>& condition)
	{
		return Query(query::MakeSelectValuesQuery(name_table, name_columns, condition));
	}

	void ODBC::InsertValues(const std::string& name_table, const std::vector<std::string>& name_columns, const std::vector<std::string>& values)
	{
		Query(query::MakeInsertValuesQuery(name_table, name_columns, values));
	}

	void ODBC::UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns, const std::vector<std::string>& values, const std::vector<std::string>& condition)
	{
		Query(query::MakeUpdateValuesQuery(name_table, name_columns, values, condition));
	}

	void ODBC::CreateTable(const std::string& name_table, const std::string& name_column, const std::string& type, const std::string& value, const std::vector<std::string>& attributes)
	{
		std::string query = query::MakeCreateTableQuery(name_table, name_column, type, value, attributes);

		query.erase(query.find("DEFAULT CHARSET utf8"), 20);

		Query(query);
	}

	void ODBC::CreateColumn(const std::string& name_table, const std::string& name_column, const std::string& type, const std::string& value, const std::vector<std::string>& attributes)
	{
		Query(query::MakeCreateColumnQuery(name_table, name_column, type, value, attributes));
	}

	void ODBC::ModifyColumn(const std::string& name_table, const std::string& name_column, const std::string& type, const std::string& value, const std::vector<std::string>& attributes)
	{
		std::string query = query::MakeModifyColumnQuery(name_table, name_column, type, value, attributes);

		size_t pos = query.find("MODIFY");
		query.erase(pos, 6);
		query.insert(pos, "ALTER");

		Query(query);
	}

	void ODBC::DeleteTable(const std::string& name_table)
	{
		Query(query::MakeDeleteTableQuery(name_table));
	}

	void ODBC::DeleteColumn(const std::string& name_table, const std::string& name_column)
	{
		Query(query::MakeDeleteColumnQuery(name_table, name_column));
	}

	void ODBC::DeleteValues(const std::string& name_table, const std::string& condition)
	{
		Query(query::MakeDeleteValuesQuery(name_table, condition));
	}
	//мда уж, ебанутые приведения типов... UNICODE в пролёте однозначнов

	void ODBC::Exit()
	{
		if (hStmt)
			SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

		if (hDbc)
		{
			SQLDisconnect(hDbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
		}

		if (hEnv)
			SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
	}
} // namespace odbc
