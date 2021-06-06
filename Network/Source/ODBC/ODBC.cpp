#include <pch.h>
#include "ODBC/ODBC.h"
#include "SQL_Query.hpp"
#include "boost/algorithm/string/case_conv.hpp"
#include <odbcinst.h>

#include <atlconv.h>
namespace odbc
{
	void ODBC::CreateDataBase(const std::string &driver, const std::string &path, const std::string &attributes,
		const std::string &password)
	{
		USES_CONVERSION;
		if (!SQLConfigDataSourceW(nullptr, ODBC_ADD_DSN, A2W(driver.c_str()),
			A2W(("CREATE_DB=\"" + path + "\";" + attributes + (password.empty() ? "" : ";PWD=" + password)).c_str())))
			Logger_Error_F("Something is wrong with create a database file, error code: %i", GetLastError());
	}

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

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLDriverConnectA(hDbc, nullptr,
			(SQLCHAR*)(("Driver={" + driver + "};Dbq=" + path + ";" + attributes + ";PWD=" + password + ";").c_str()),
			SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT)))
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
		RETCODE rc = SQLExecDirectA(hStmt, (SQLCHAR*)(query.c_str()), SQL_NTS);
		json res = {};
		SQLLEN nOldArraySize = 0, nOldRowsetSize = 0;
		rc = SQLGetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, &nOldArraySize, sizeof(nOldArraySize), NULL);
		rc = SQLGetStmtAttr(hStmt, SQL_ROWSET_SIZE, &nOldRowsetSize, sizeof(nOldArraySize), NULL);
		rc = SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)1, 0);
		rc = SQLSetStmtAttr(hStmt, SQL_ROWSET_SIZE, (PTR)1, 0);

		switch (rc)
		{

		case SQL_SUCCESS_WITH_INFO:
		{
			PrintError(hStmt, SQL_HANDLE_STMT, rc);
			// fall through
		}

		case SQL_SUCCESS:
		{
			SQLSMALLINT sNumResults = 0;
			PrintError(hStmt, SQL_HANDLE_STMT, SQLNumResultCols(hStmt, &sNumResults));
			std::vector<lpGETINFOALL> lpgi;
			std::vector<std::string> columnNames = {};

			rc = SQLNumResultCols(hStmt, &sNumResults);
			if ((rc) != SQL_SUCCESS && (rc) != SQL_SUCCESS_WITH_INFO)
				break;

			for (SQLSMALLINT i = 0; i < sNumResults; i++)
			{
				auto dwReqdMem = sizeof(GETINFOALL) * (DWORD)sNumResults;	// Explicit promotion required
				lpGETINFOALL newObj = (lpGETINFOALL)calloc(sNumResults, dwReqdMem);

				rc = SQLDescribeCol(hStmt,
					(UWORD)(i + 1),
					(LPTSTR)newObj->szCol,
					0, NULL,
					&newObj->fSqlType,
					&newObj->cbValueMax,
					NULL, NULL);

				if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
					break;

				UINT cbChar = sizeof(TCHAR);
				switch (newObj->fSqlType)
				{
				case SQL_BINARY:
				case SQL_VARBINARY:
				case SQL_LONGVARBINARY:
					// Binary types must allow for twice as much room for the char display
					if (newObj->cbValueMax == 0)
						//Handle MAX 
						newObj->cbValueMax = 8000;
					else {
						newObj->cbValueMax *= (2 * cbChar) + cbChar;
						newObj->fSqlType = SQL_BINARY;
					}
					break;
				case SQL_CHAR:
				case SQL_VARCHAR:
				case SQL_LONGVARCHAR:
				case SQL_WCHAR:
				case SQL_WVARCHAR:
				case SQL_WLONGVARCHAR:
				{
					newObj->fSqlType = SQL_CHAR;
					// Worst case, each Unicode char maps to a double-byte char
					// Prevent overflow if value is half a gig or larger
					if (newObj->cbValueMax < 0x7fffffff)
					{
						newObj->cbValueMax *= 2;
						newObj->cbValueMax += cbChar;
					}
					else
						newObj->cbValueMax = 0xffffffff;
				}
				break;
				default:
					// For other types, use a default buffer size
					newObj->cbValueMax = 100;

				} //switch(fSqlType)

				newObj->cbValueMax = (newObj->cbValueMax < (UWORD)(-1) ? newObj->cbValueMax : (UWORD)(-1));
				newObj->rgbValue = (PTR)alloca(newObj->cbValueMax);
								
				char buf[8192];
				PrintError(hStmt, SQL_HANDLE_STMT, SQLColAttributeA(hStmt, i + 1, SQL_COLUMN_NAME, buf,
					sizeof(buf), nullptr, nullptr));
				columnNames.push_back(buf);

				lpgi.push_back(newObj);
			}

			while ((rc = SQLFetch(hStmt)) == SQL_STILL_EXECUTING)
			{
			}

			rc = SQL_SUCCESS;

			while ((rc) == SQL_SUCCESS || (rc) == SQL_SUCCESS_WITH_INFO)
			{
				for (size_t i = 0; i < lpgi.size(); i++)
				{
					SQLLEN cbValue;
					PrintError(hStmt,
						SQL_HANDLE_STMT, rc = SQLGetData(hStmt,
						(UWORD)(i + 1),
							SQL_C_CHAR,
							lpgi.at(i)->rgbValue,
							lpgi.at(i)->cbValueMax,
							&cbValue));
					while (rc == SQL_STILL_EXECUTING)
					{
					}

					if (((rc) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO && rc != SQL_NO_DATA_FOUND) &&
						lpgi.at(i)->rgbValue != nullptr)
					{
						LPSTR buf = (LPSTR)lpgi.at(i)->rgbValue;

						if (cbValue == SQL_NULL_DATA)
						{
							res[columnNames[i]].push_back(json()); // Was "NULL"
							continue;
						}

						switch (lpgi.at(i)->fSqlType)
						{
						case SQL_BIT:
						case SQL_INTEGER:
						case SQL_NUMERIC:
						case SQL_TINYINT:
						case SQL_SMALLINT:
						case SQL_BIGINT:
							res[columnNames[i]].push_back(atoi(buf));
							break;
						case SQL_REAL:
						case SQL_DECIMAL:
						case SQL_DOUBLE:
							res[columnNames[i]].push_back(atof(buf));
							break;

						case SQL_CHAR:
						case SQL_VARCHAR:
						case SQL_LONGVARCHAR:
						case SQL_WCHAR:
						case SQL_WVARCHAR:
						case SQL_WLONGVARCHAR:
						{
							std::string str = buf;
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
								}
								else if (_js.is_array())
									res[columnNames[i]].push_back(json({ _js })[0]);
							}
							res[columnNames[i]].push_back(str.empty() ? "" : _js);
						}
						}
					}
				}

				if ((rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) && rc != SQL_NO_DATA_FOUND)
					break;
				while ((rc = SQLFetch(hStmt)) == SQL_STILL_EXECUTING) {}
			}
			break;
		}
		case SQL_ERROR:
		{
			PrintError(hStmt, SQL_HANDLE_STMT, rc);
			break;
		}

		default:
#if defined(HAS_LOGGER)
			Logger_Critical_F("Unexpected return code %hd!\n", rc);
#endif
		}

		PrintError(hStmt, SQL_HANDLE_STMT, SQLFreeStmt(hStmt, SQL_CLOSE));

		//Reset rowset sizes
		rc = SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)(LONG_PTR)nOldArraySize, sizeof(nOldArraySize));
		rc = SQLSetStmtAttr(hStmt, SQL_ROWSET_SIZE, (PTR)(LONG_PTR)nOldRowsetSize, sizeof(nOldArraySize));

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

	void ODBC::CreateTable(const std::string& name_table, const std::vector<std::string>& name_column, const std::vector<std::string>& type,
		const std::vector<std::string>& value, const std::vector<std::vector<std::string>>& attributes)
	{
		std::string query = query::MakeCreateTableQuery(name_table, name_column, type, value, attributes);

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

	std::pair<bool, std::vector<std::string>> ODBC::GetListTablesDatabase()
	{
		std::pair<bool, std::vector<std::string>> ret = { false, {} };
		std::vector<lpGETINFOALL> lpgi;
		DWORD dwReqdMem;
		SQLSMALLINT cCols;
		SQLRETURN rc = SQL_SUCCESS;
		// Invoke function
		SQLLEN nOldArraySize = 0, nOldRowsetSize = 0;

		rc = SQLGetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, &nOldArraySize, sizeof(nOldArraySize), NULL);
		rc = SQLGetStmtAttr(hStmt, SQL_ROWSET_SIZE, &nOldRowsetSize, sizeof(nOldArraySize), NULL);
		rc = SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)1, 0);
		rc = SQLSetStmtAttr(hStmt, SQL_ROWSET_SIZE, (PTR)1, 0);

		rc = SQLTables(hStmt,
			SQL_NULL_HANDLE,											// szTableQualifier
			0,            				// cbTableQualifier
			SQL_NULL_HANDLE,                   				// szTableOwner
			SQL_NULL_HANDLE,            				// cbTableOwner
			0,                   				// szTableName
			SQL_NULL_HANDLE,          					// cbTableName
			0,                   				// szTableType
			SQL_NULL_HANDLE);          					// cbTableType

		rc = SQLNumResultCols(hStmt, &cCols);
		if ((rc) != SQL_SUCCESS && (rc) != SQL_SUCCESS_WITH_INFO)
			return ret;

		for (SQLSMALLINT i = 0; i < cCols; i++)
		{
			dwReqdMem = sizeof(GETINFOALL) * (DWORD)cCols;	// Explicit promotion required
			lpGETINFOALL newObj = (lpGETINFOALL)calloc(cCols, dwReqdMem);

			rc = SQLDescribeCol(hStmt,
				(UWORD)(i + 1),
				(LPTSTR)newObj->szCol,
				0, NULL,
				&newObj->fSqlType,
				&newObj->cbValueMax,
				NULL, NULL);

			if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
				return ret;

			UINT cbChar = sizeof(TCHAR);
			switch (newObj->fSqlType)
			{
			case SQL_BINARY:
			case SQL_VARBINARY:
			case SQL_LONGVARBINARY:
				// Binary types must allow for twice as much room for the char display
				if (newObj->cbValueMax == 0)
					//Handle MAX 
					newObj->cbValueMax = 8000;
				else {
					newObj->cbValueMax *= (2 * cbChar) + cbChar;
					newObj->fSqlType = SQL_BINARY;
				}
				break;
			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_LONGVARCHAR:
			case SQL_WCHAR:
			case SQL_WVARCHAR:
			case SQL_WLONGVARCHAR:
			{
				newObj->fSqlType = SQL_CHAR;
				// Worst case, each Unicode char maps to a double-byte char
				// Prevent overflow if value is half a gig or larger
				if (newObj->cbValueMax < 0x7fffffff)
				{
					newObj->cbValueMax *= 2;
					newObj->cbValueMax += cbChar;
				}
				else
					newObj->cbValueMax = 0xffffffff;
			}
			break;
			default:
				// For other types, use a default buffer size
				newObj->cbValueMax = 100;

			} //switch(fSqlType)

			newObj->cbValueMax = (newObj->cbValueMax < (UWORD)(-1) ? newObj->cbValueMax : (UWORD)(-1));
			newObj->rgbValue = (PTR)alloca(newObj->cbValueMax);

			lpgi.push_back(newObj);
		}

		while ((rc = SQLFetch(hStmt)) == SQL_STILL_EXECUTING)
		{
		}

		rc = SQL_SUCCESS;

		while ((rc) == SQL_SUCCESS || (rc) == SQL_SUCCESS_WITH_INFO)
		{
			for (size_t i = 1; i < lpgi.size(); i++)
			{
				SQLLEN cbValue;
				PrintError(hStmt,
					SQL_HANDLE_STMT, rc = SQLGetData(hStmt,
					(UWORD)(i + 1),
						SQL_C_CHAR,
						lpgi.at(i)->rgbValue,
						lpgi.at(i)->cbValueMax,
						&cbValue));
				while (rc == SQL_STILL_EXECUTING)
				{
				}

				if (cbValue < 0)
					continue;

				if (((rc) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO && rc != SQL_NO_DATA_FOUND) &&
					lpgi.at(i)->rgbValue != nullptr)
				{
					std::string cur_itm = (LPSTR)lpgi.at(i)->rgbValue;
					ret.second.push_back(cur_itm);
					if (cur_itm == "SYSTEM TABLE" || cur_itm == "TABLE")
					{
						if (cur_itm != "TABLE")
						{
							for (size_t _ = 0; _ < 2; _++)
							{
								if (!ret.second.empty())
									ret.second.pop_back();
							}
						}
						else
						{
							auto ptr = ret.second.begin();
							while (ptr < ret.second.end())
							{
								if (*ptr == "TABLE")
								{
									ptr = ret.second.erase(ptr);
									ret.first = true;
								}
								else
									ptr++;
							}
						}
					}
				}
			}

			if ((rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) && rc != SQL_NO_DATA_FOUND)
				break;

			while ((rc = SQLFetch(hStmt)) == SQL_STILL_EXECUTING)
			{
			}
		}

		SQLFreeStmt(hStmt, SQL_CLOSE);

		//Reset rowset sizes
		rc = SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)(LONG_PTR)nOldArraySize, sizeof(nOldArraySize));
		rc = SQLSetStmtAttr(hStmt, SQL_ROWSET_SIZE, (PTR)(LONG_PTR)nOldRowsetSize, sizeof(nOldArraySize));

		return ret;
	}
} // namespace odbc
