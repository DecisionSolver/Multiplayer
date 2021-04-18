#include <pch.h>
#include "ODBC/ODBC.h"
#include "SQL_Query.hpp"
#include "boost/algorithm/string/case_conv.hpp"

namespace odbc
{
	void ODBC::Connect(const std::string& driver, const std::string& path,
		const std::string& attributes, const std::string& password)
	{
		if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
		{
			Logger_Error("Unable to allocate an environment handle\n");
			return;
		}

		if (PrintError(hEnv, SQL_HANDLE_ENV, SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, 0)))
			return;

		if (PrintError(hEnv, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)))
			return;

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLDriverConnectA(hDbc, NULL,
			(SQLCHAR*)(("Driver={" + driver + "};Dbq=" + path + ";" + attributes + ";PWD=" + password + ";").c_str()),
			SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT)))
			return;

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)))
			return;
	}

	bool ODBC::PrintError(SQLHANDLE hHandle, SQLSMALLINT hType, SQLRETURN e)
	{
		SQLSMALLINT iRec = 0;
		SQLINTEGER  iError = 0;
		SQLCHAR     wszMessage[1000], wszState[SQL_SQLSTATE_SIZE + 1];

		if (e == SQL_INVALID_HANDLE)
		{

#if defined(HAS_LOGGER)
			Logger_Critical("Invalid handle!\n");
#endif 
			return true;
		}

		while (SQLGetDiagRecA(hType, hHandle, ++iRec, wszState, &iError, wszMessage, (SQLSMALLINT)(sizeof(wszMessage) /
			sizeof(CHAR)), 0) == SQL_SUCCESS)
		{
			// Hide data truncated..
			if (!strcmp((const char*)wszState, "01000"))
			{
#if defined(HAS_LOGGER)
				Logger_Warn_F("[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
#endif
				return false;
			}

#if defined(HAS_LOGGER)
				Logger_Error_F("[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
#endif
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
			SQLSMALLINT sNumResults = 0;
			PrintError(hStmt, SQL_HANDLE_STMT, SQLNumResultCols(hStmt, &sNumResults));

			if (sNumResults > 0)
			{
				char buf[8192], type[100];
				std::vector<std::pair<std::string, std::string>> columnNames = {};

				for (SQLSMALLINT i = 1; i <= sNumResults; i++)
				{
					PrintError(hStmt, SQL_HANDLE_STMT, SQLColAttributeA(hStmt, i, SQL_COLUMN_NAME, buf, sizeof(buf), NULL, NULL));
					PrintError(hStmt, SQL_HANDLE_STMT, SQLColAttributeA(hStmt, i, SQL_COLUMN_TYPE_NAME, type, sizeof(type), NULL, NULL));
					columnNames.push_back({ buf, type });
				}
				while (SQLFetch(hStmt) == SQL_SUCCESS)
				{
					for (SQLSMALLINT i = 1; i <= sNumResults; i++)
					{
						SQLINTEGER indicator = 0;

						if (SQLGetData(hStmt, i, SQL_C_CHAR, buf, sizeof(buf), &indicator) == SQL_SUCCESS)
						{
							if (indicator == SQL_NULL_DATA)
							{
								res[columnNames[i - 1].first].push_back(json()); // Was "NULL"
								continue;
							}
							if (columnNames[i - 1].second == "BIT" ||
								columnNames[i - 1].second == "INTEGER" ||
								columnNames[i - 1].second == "NUMERIC" ||
								columnNames[i - 1].second == "TINYINT" ||
								columnNames[i - 1].second == "SMALLINT" ||
								columnNames[i - 1].second == "BIGINT")
								res[columnNames[i - 1].first].push_back(atoi(buf));
							else if (columnNames[i - 1].second == "REAL" ||
								columnNames[i - 1].second == "DECIMAL" ||
								columnNames[i - 1].second == "DOUBLE")
								res[columnNames[i - 1].first].push_back(atof(buf));
							else if (columnNames[i - 1].second == "CHAR" ||
								columnNames[i - 1].second == "VARCHAR" ||
								columnNames[i - 1].second == "LONGVARCHAR" ||
								columnNames[i - 1].second == "BINARY" ||
								columnNames[i - 1].second == "VARBINARY" ||
								columnNames[i - 1].second == "LONGVARBINARY"||
								columnNames[i - 1].second == "LONGCHAR")
							{
								std::string str = buf;

								// To Avoid If It Is Not String At All (Like Number) Because JSON Parse "STRING"
								// From Only Numbers Like NUMBER type!
								if (!str.empty() && (str.front() != '"' && str.back() != '"'))
								{
									str.insert(str.begin(), '"');
									str.insert(str.end(), '"');
								}

								json _js;
								if (!str.empty())
								{
									try
									{
										_js = json::parse(str);
									}
									catch (json::exception)
									{
										_js = str;
									}
									if (!_js.empty() && _js.is_object())
									{
										for (auto&[key, val]: _js.items())
										{
											for (auto&[_, elm]: val.items())
											{
												res[key].push_back(elm);
											}
										}
										break;
									}
									else if (_js.is_array())
									{
										res[columnNames[i - 1].first].push_back(json({ _js })[0]);
										break;
									}
								}
								res[columnNames[i - 1].first].push_back(str.empty() ? "" : _js);
								break;
							}
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
#if defined(HAS_LOGGER)
			Logger_Critical_F("Unexpected return code %hd!\n", RetCode);
#endif
		}

		PrintError(hStmt, SQL_HANDLE_STMT, SQLFreeStmt(hStmt, SQL_CLOSE));

		return res;
	}

	nlohmann::json ODBC::SelectValues(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& condition)
	{
		return Query(query::MakeSelectValuesQuery(name_table, name_columns, condition));
	}

	void ODBC::InsertValues(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& values)
	{
		Query(query::MakeInsertValuesQuery(name_table, name_columns, values));
	}

	void ODBC::UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& values, const std::vector<std::string>& condition)
	{
		Query(query::MakeUpdateValuesQuery(name_table, name_columns, values, condition));
	}

	void ODBC::CreateTable(const std::string& name_table, const std::string& name_column, const std::string& type,
		const std::string& value, const std::vector<std::string>& attributes)
	{
		std::string query = query::MakeCreateTableQuery(name_table, name_column, type, value, attributes);
		boost::to_upper(query);

		query.erase(query.find("DEFAULT CHARSET UTF8"), 20);

		Query(query);
	}

	void ODBC::CreateColumn(const std::string& name_table, const std::string& name_column, const std::string& type,
		const std::string& value, const std::vector<std::string>& attributes)
	{
		Query(query::MakeCreateColumnQuery(name_table, name_column, type, value, attributes));
	}

	void ODBC::ModifyColumn(const std::string& name_table, const std::string& name_column, const std::string& type,
		const std::string& value, const std::vector<std::string>& attributes)
	{
		std::string query = query::MakeModifyColumnQuery(name_table, name_column, type, value, attributes);
		boost::to_upper(query);

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
