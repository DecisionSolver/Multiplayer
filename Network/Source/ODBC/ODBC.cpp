#include <pch.h>

#include <windows.h>

#include "ODBC/ODBC.h"
#include "DB_Query.hpp"
#include "boost/algorithm/string/case_conv.hpp"
#include <odbcinst.h>

#include <sql.h>
#include <sqlext.h>

#include <atlconv.h>
namespace odbc
{
	void ODBC::CreateDataBase(const std::string &driver, const std::string &path, const std::string &attributes,
		const std::string &password) const
	{
		USES_CONVERSION;
		if ((SQLConfigDataSourceW(nullptr, ODBC_ADD_DSN, A2W(driver.c_str()),
			A2W(("CREATE_DB=\"" + path + "\";" + attributes + (password.empty() ? "" : ";PWD=" + password)).c_str()))) != 1)
			Logger_Error_F("Something is wrong with create a database file, error code: {}", GetLastError());
	}

	bool ODBC::Connect(const std::string& driver, const std::string& path, const std::string& Table,
		const std::vector<std::string>& attributes, const std::string& password)
	{
		if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
		{
			Logger_Error("Unable to allocate an environment handle\n");
			return false;
		}

		if (PrintError(hEnv, SQL_HANDLE_ENV, SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, 0)))
			return false;

		if (PrintError(hEnv, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)))
			return false;

		std::string allattributes = "";
		for (auto attribute : attributes)
			allattributes += attribute + ";";

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLDriverConnectA(hDbc, nullptr,
			(SQLCHAR*)(("Driver={" + driver + "};Dbq=" + path + ";" + allattributes + "PWD=" + password + ";").c_str()),
			SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT)))
			return false;

		SetCurrentTable(Table);

		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)))
			return false;

		return true;
	}
	bool ODBC::PrintError(SQLHANDLE hHandle, SQLSMALLINT hType, SQLRETURN e) const
	{
		SQLSMALLINT iRec = 0;
		SQLINTEGER  iError = 0;
		SQLCHAR     wszMessage[1000], wszState[SQL_SQLSTATE_SIZE + 1];

		if (e == SQL_INVALID_HANDLE)
		{
#if __has_include("logger.h")
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
#if __has_include("logger.h")
				Logger_Warn_F("[{}] {} ({})\n", wszState, wszMessage, iError);
#endif
				return false;
			}

#if __has_include("logger.h")
				Logger_Error_F("[{}] {} ({})\n", wszState, wszMessage, iError);
#endif
		}

		return false;
	}
	int ODBC::GetCntData(const std::string & query) const
	{
		SQLHSTMT Local = nullptr;
		if (PrintError(hDbc, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &Local)))
			return 0;
		RETCODE rc = SQLExecDirectA(Local, (SQLCHAR*)(query.c_str()), SQL_NTS);


		SQLLEN cnt = 0;
		while ((rc = SQLFetch(Local)) != SQL_NO_DATA)
		{
			cnt++;
		}
		
		PrintError(Local, SQL_HANDLE_STMT, SQLFreeStmt(Local, SQL_CLOSE));
		
		SQLFreeHandle(SQL_HANDLE_STMT, Local);

		return cnt;
	}
	nlohmann::json ODBC::Query(const std::string& query, bool Need_SQL_TYPE) const
	{
		RETCODE rc = SQLExecDirectA(hStmt, (SQLCHAR*)(query.c_str()), SQL_NTS);
		json res = {};
		SQLLEN nOldArraySize = 0, nOldRowsetSize = 0;

		switch (rc)
		{
		case SQL_SUCCESS_WITH_INFO:
		{
			PrintError(hStmt, SQL_HANDLE_STMT, rc);
			// fall through
		}

		case SQL_SUCCESS:
		{
			rc = SQLGetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, &nOldArraySize, sizeof(nOldArraySize), NULL);
			rc = SQLGetStmtAttr(hStmt, SQL_ROWSET_SIZE, &nOldRowsetSize, sizeof(nOldArraySize), NULL);
			rc = SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)1, 0);
			rc = SQLSetStmtAttr(hStmt, SQL_ROWSET_SIZE, (PTR)1, 0);

			SQLSMALLINT sNumResults = 0;
			PrintError(hStmt, SQL_HANDLE_STMT, SQLNumResultCols(hStmt, &sNumResults));
			std::vector<lpGETINFOALL> lpgi;
			std::vector<std::pair<std::string, std::string>> columnNames = {};

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
								
				char buf[8192], type[255];
				PrintError(hStmt, SQL_HANDLE_STMT, SQLColAttributeA(hStmt, i + 1, SQL_COLUMN_NAME, buf,
					sizeof(buf), nullptr, nullptr));
				PrintError(hStmt, SQL_HANDLE_STMT, SQLColAttributeA(hStmt, i + 1, SQL_COLUMN_TYPE_NAME,
					type, sizeof(type), NULL, NULL));
				columnNames.push_back({ buf, type });

				lpgi.push_back(newObj);
			}
			if (Need_SQL_TYPE)
			{
				for (size_t i = 0; i < columnNames.size(); i++)
				{
					res[columnNames[i].first].push_back({ {"sql_type", columnNames[i].second} });
				}
			}

			while ((rc = SQLFetch(hStmt)) == SQL_STILL_EXECUTING) {}

			// Do A New SQLExecuteDirect To Get How Much Count Of Data Will Be
			SQLLEN cnt = 1;
			if (query.find("SELECT") != std::string::npos)
				cnt = GetCntData(query);
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
							if (cnt == 1 && sNumResults == 1)
								res = json();
							else
								res[columnNames[i].first].push_back(json()); // Was "NULL"
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
						{
							if (cnt == 1 && sNumResults == 1)
								res = json::object({ { columnNames[i].first, atoi(buf) } });
							else
								res[columnNames[i].first].push_back(atoi(buf));
							break;
						}
						case SQL_REAL:
						case SQL_DECIMAL:
						case SQL_DOUBLE:
						{
							if (cnt == 1 && sNumResults == 1)
								res = json::object({ { columnNames[i].first, atof(buf) } });
							else
								res[columnNames[i].first].push_back(atof(buf));
							break;
						}
						case SQL_CHAR:
						case SQL_VARCHAR:
						case SQL_LONGVARCHAR:
						case SQL_WCHAR:
						case SQL_WVARCHAR:
						case SQL_WLONGVARCHAR:
						{
							std::string str = buf;

							std::string::size_type size;
							try
							{
								// It Needs Only For Indicate If It's Number Or Not!
								std::stoi(str, &size);

								// If In This String Has Something Else With Numbers (Can consider it not number, it's string!)
								if (size != str.length())
									size = std::string::npos;
							}
							// No numbers in that string
							catch (const std::exception&)
							{
								size = std::string::npos;
							}

							// To Avoid If It Is Not String At All (Like Number) Because JSON Parse "STRING"
							// From Only Numbers Like NUMBER type!
							if (size != std::string::npos)
							{
								str.erase(str.begin());
								str.erase(str.end());
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
							}
							if (cnt == 1 && sNumResults == 1)
								res = _js;
							else
								res[columnNames[i].first].push_back(str.empty() ? "" : _js);
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

		// In Somecase it may consider like success!
		case SQL_NO_DATA_FOUND:
			break;

		default:
#if __has_include("logger.h")
			Logger_Error_F("Unexpected return code {}!\n", rc);
#endif
		}

		PrintError(hStmt, SQL_HANDLE_STMT, SQLFreeStmt(hStmt, SQL_CLOSE));

		//Reset rowset sizes
		rc = SQLSetStmtAttr(hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (PTR)(LONG_PTR)nOldArraySize, sizeof(nOldArraySize));
		rc = SQLSetStmtAttr(hStmt, SQL_ROWSET_SIZE, (PTR)(LONG_PTR)nOldRowsetSize, sizeof(nOldArraySize));

		return res;
	}

	nlohmann::json ODBC::SelectValues(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& condition, bool Need_SQL_TYPE) const
	{
		return Query(query::MakeSelectValuesQuery(!name_table.empty() ? name_table : CurrentTable, 
			name_columns, condition), Need_SQL_TYPE);
	}

	void ODBC::InsertValues(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& values) const
	{
		Query(query::MakeInsertValuesQuery(!name_table.empty() ? name_table : CurrentTable, 
			name_columns, values));
	}

	void ODBC::UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& values, const std::vector<std::string>& condition) const
	{
		Query(query::MakeUpdateValuesQuery(!name_table.empty() ? name_table : CurrentTable, 
			name_columns, values, condition));
	}

	void ODBC::CreateTable(const std::string& name_table, const std::vector<std::string>& name_column,
		const std::vector<std::string>& type, const std::vector<std::string>& value,
		const std::vector<std::vector<std::string>>& attributes) const
	{
		std::string ret_query = query::MakeCreateTableQuery(!name_table.empty() ? name_table : CurrentTable, 
			name_column, type, value, attributes);

		if (ret_query.empty())
			return;

		ret_query.erase(ret_query.find("DEFAULT CHARSET UTF8"), 20);

		Query(ret_query);
	}

	void ODBC::CreateColumn(const std::string& name_table, const std::string& name_column, const std::string& type,
		const std::string& value, const std::vector<std::string>& attributes) const
	{
		Query(query::MakeCreateColumnQuery(!name_table.empty() ? name_table : CurrentTable, 
			name_column, type, value, attributes));
	}

	void ODBC::ModifyColumn(const std::string& name_table, const std::string& name_column, const std::string& type,
		const std::string& value, const std::vector<std::string>& attributes) const
	{
		std::string ret_query = query::MakeModifyColumnQuery(!name_table.empty() ? name_table : CurrentTable, 
			name_column, type, value, attributes);

		if (ret_query.empty())
			return;

		size_t pos = ret_query.find("MODIFY");
		ret_query.erase(pos, 6);
		ret_query.insert(pos, "ALTER");

		Query(ret_query);
	}

	void ODBC::DeleteTable(const std::string& name_table) const
	{
		Query(query::MakeDeleteTableQuery(!name_table.empty() ? name_table : CurrentTable));
	}

	void ODBC::DeleteColumn(const std::string& name_table, const std::string& name_column) const
	{
		Query(query::MakeDeleteColumnQuery(!name_table.empty() ? name_table : CurrentTable, name_column));
	}

	void ODBC::DeleteValues(const std::string& name_table, const std::string& condition) const
	{
		Query(query::MakeDeleteValuesQuery(!name_table.empty() ? name_table : CurrentTable, condition));
	}

	void ODBC::Exit() const
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

	std::pair<bool, std::vector<std::string>> ODBC::GetListTablesDatabase() const
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

	void ODBC::SplitDB(const std::string &NameTable, const std::string &NameNewFile) const
	{
		if (!hEnv || !hDbc || !hStmt)
		{
			Logger_Error("This Function Requires To Be Connected To Needed DB To Continue! Aborting!");
			return;
		}

		if (NameTable.empty() || NameNewFile.empty())
		{
			Logger_Error("This Function Requires To Have Not Empty Params To Work! Aborting!");
			return;
		}

		std::string c_str = "Microsoft Access Driver (*.mdb)";

		auto SecondFile = std::make_shared<ODBC>();
		// Create A New DB
		SecondFile->CreateDataBase(c_str, NameNewFile);
		SecondFile->Connect(c_str, NameNewFile, "", {});

		std::string table = !NameTable.empty() ? NameTable : CurrentTable;
		// Then Get * From NameTable And Add It To New DB
		auto CurrDB = SelectValues(table, { "*" }, {}, true);

		std::vector<std::string> NameColumns, TypeColumns, EmptyValue;
		std::vector<std::vector<std::string>> EmptyAttributes;
		for (auto it = CurrDB.begin(); it != CurrDB.end(); ++it)
		{
			NameColumns.push_back(it.key());

			if (!it.value().front()["sql_type"].is_null() || !it.value().front()["sql_type"].empty())
				TypeColumns.push_back(it.value().front()["sql_type"]);

			EmptyValue.push_back("");
			EmptyAttributes.push_back({});
		}
		SecondFile->CreateTable(table, NameColumns, TypeColumns, EmptyValue, EmptyAttributes);

		size_t size_y = 
			(size_t)Query("SELECT COUNT(*) FROM `" + table + "`").front().back().get<json::number_integer_t>(),
			cur = 1u;
		
		while (true)
		{
			std::vector<std::string> columns, value;
			for (size_t i = 0; i < NameColumns.size(); i++)
			{
				columns.push_back(NameColumns.at(i));

				auto It = CurrDB[NameColumns.at(i)];
				if (It.is_string())
					value.push_back(It);
				else if (It.is_array())
					value.push_back(It.at(cur));
				else if (It.is_object())
					value.push_back(It);
			}
			SecondFile->InsertValues(NameTable, columns, value);
			if (cur >= size_y)
				break;
			else
				cur++;
		}
		SecondFile->Exit();
	}
} // namespace odbc
