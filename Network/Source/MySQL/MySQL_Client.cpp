#include <pch.h>
#include "MySQL/MySQL_Client.h"
#include "DB_Query.hpp"

sql::ConnectOptionsMap mysql::Client::connection_properties;
std::shared_ptr<sql::Connection> mysql::Client::connection;

namespace mysql
{
	/* returns True when the exception was caught */
	bool Client::TryException(const std::function<void()> &functionToCatch, const std::string &UserData)
	{
		try
		{
			functionToCatch();
		}
		catch (const sql::SQLException &exception)
		{
			// "No result available"
			if (exception.getErrorCode() == 0)
			{
				return false;
			}

	#if __has_include("logger.h")
			Logger_Error_F("Error Occured: \"{}\", Error Code: {}, SQL State: \"{}\"",
				exception.what(), exception.getErrorCode(), exception.getSQLStateCStr());
	#endif

			// If "Lost Connection To MySQL Server During Query" or "Can't connect to MySQL server"
			if (exception.getErrorCode() == 2013 || exception.getErrorCode() == 2003)
			{
#if __has_include("logger.h")
				Logger_Info("Made Reconnect To Server");
#endif
				if (connection && !connection->isValid())
				{
					connection->reconnect();

					functionToCatch();
				}
			}
			return true;
		}
		catch (const nlohmann::json::exception &exception)
		{
	#if __has_include("logger.h")
			Logger_Error_F("Error Occured: \"{}\"", exception.what());
	#endif
			return true;
		}

		return false;
	}

	void Client::SetCurrentDatabase(const std::string &DataBase)
	{
		if (!DataBase.empty() && connection)
		{
			CurrentDatabase = DataBase;
			connection->setSchema(CurrentDatabase.c_str());
			connection_properties["schema"] = CurrentDatabase;
		}
	}

	Client::Status Client::Connect(const std::string &user, const std::string &password, const std::string &host,
		const std::string &Database, const std::string &Table, const unsigned short &port, const std::string &charset,
		bool ReadOnly)
	{
		if (!TryException([&]()
		{
			connection_properties["hostName"] = host;
			connection_properties["userName"] = user;
			connection_properties["password"] = password;
			connection_properties["port"] = port;
			connection_properties["OPT_RECONNECT"] = true;
			connection_properties["OPT_CLIENT_MULTI_STATEMENTS "] = true;
			connection_properties["OPT_RETRY_COUNT "] = 15;

			if (!charset.empty())
			{
				connection_properties["OPT_CHARSET_NAME"] = charset;
			}
			if (!Database.empty())
			{
				connection_properties["schema"] = Database;
			}

			connection.reset(get_driver_instance()->connect(connection_properties));

#if defined (_DEBUG)
			int on_off = 1;
			connection->setClientOption("libmysql_debug", "d:t:O,client.trace");
			connection->setClientOption("clientTrace", &on_off);
#endif
			isReadOnly = ReadOnly;

			SetCurrentDatabase(Database);
			SetCurrentTable(Table);
		}))
		{
			return Client::Status::Done;
		}
		else
		{
			return Client::Status::Error;
		}
	}
	
	sql::ResultSet *Client::Query(const std::string &query)
	{
		get_driver_instance()->threadInit();

		sql::ResultSet *Result = nullptr;
		sql::Statement *statement = connection->createStatement();
		TryException([&]()
		{
			if (!connection)
			{
				connection.reset(get_driver_instance()->connect(connection_properties));

				Result = Query(query);
			}
			else
			{
				std::string NewQuery = query;
				if (NewQuery.back() != ';')
				{
					NewQuery.push_back(';');
				}
				else if (std::count(NewQuery.begin(), NewQuery.end(), ';') > 2)
				{
					NewQuery.erase(NewQuery.find(';', 1));
				}

				Result = statement->executeQuery(NewQuery.c_str());
			}
		}, query);

		delete statement;
		
		get_driver_instance()->threadEnd();

		return Result;
	}
	
	void Client::Exec(const std::string &query)
	{
		Query(query);
	}

	void Client::CreateDatabaseAndSetCurrent(const std::string &name_database)
	{
		Exec(query::MakeCreateDatabaseQuery(name_database));

		SetCurrentDatabase(name_database);
	}

	void Client::DeleteDatabase(const std::string &name_database)
	{
		Exec(query::MakeDropDatabaseQuery(name_database));
	}

	void Client::CreateTable(const std::string &name_table, const std::vector<std::string> &name_column,
		const std::vector<std::string> &type, const std::vector<std::string> &value,
		const std::vector<std::vector<std::string>> &attributes)
	{
		Exec(query::MakeCreateTableQuery(name_table, name_column, type, value, attributes));
	}

	void Client::CreateColumn(const std::string &name_table, const std::string &name_column,
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)
	{
		Exec(query::MakeCreateColumnQuery(name_table, name_column, type, value, attributes));
	}

	void Client::CreateColumnInCurrentTable(const std::string &name_column,
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)
	{
		Exec(query::MakeCreateColumnQuery(CurrentTable, name_column, type, value, attributes));
	}

	void Client::ModifyColumn(const std::string &name_table, const std::string &name_column,
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)
	{
		Exec(query::MakeModifyColumnQuery(name_table, name_column, type, value, attributes));
	}

	void Client::ModifyColumnInCurrentTable(const std::string &name_column,
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)
	{
		Exec(query::MakeModifyColumnQuery(CurrentTable, name_column, type, value, attributes));
	}

	void Client::DeleteValues(const std::string &name_table, const std::string &condition)
	{
		Exec(query::MakeDeleteValuesQuery(name_table, condition));
	}

	void Client::DeleteValuesInCurrentTable(const std::string &condition)
	{
		Exec(query::MakeDeleteValuesQuery(CurrentTable, condition));
	}

	void Client::DeleteTable(const std::string &name_table)
	{
		if (name_table.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Exec(query::MakeDeleteTableQuery(name_table));
	}

	void Client::DeleteCurrentTable()
	{
		Exec(query::MakeDeleteTableQuery(CurrentTable));

		CurrentTable.clear();
	}

	void Client::DeleteColumn(const std::string &name_table, const std::string &name_column)
	{
		if (name_table.empty() || name_column.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Exec(query::MakeDeleteColumnQuery(name_table, name_column));
	}

	void Client::DeleteColumnInCurrentTable(const std::string &name_column)
	{
		if (name_column.empty())
		{
			throw std::exception("Not Enough Parameters!");
		}

		Exec(query::MakeDeleteColumnQuery(CurrentTable, name_column));
	}

	void Client::Destroy()
	{
		if (connection)
		{
			connection->close();
			connection.reset();
		}
	}

	nlohmann::json Client::SelectValues(const std::string &Table,
		const std::vector<std::string> &Columns, const std::vector<std::string> &Condition)
	{
		nlohmann::json ReturnJSON = {};
		if (TryException([&]()
		{
			if (!connection)
			{
				connection.reset(get_driver_instance()->connect(connection_properties));
			}

			if (!ReturnJSON.is_null())
			{
				return ReturnJSON;
			}

			std::string ProcessedColumns;
			if (Columns.back().back() != '*')
			{
				for (const auto &piece : Columns)
				{
					ProcessedColumns += "`" + piece + "`,";
				}
				ProcessedColumns.pop_back();
			}
			else
			{
				ProcessedColumns = Columns.back().back();
			}

			std::string ProcessedTable = Table;
			sql::ResultSet *ResultExec = nullptr;
			if (!Condition.empty())
			{
				std::string ProcessedConditionString = *Condition.data();
				size_t FindedPos = ProcessedColumns.find("SELECT");
				if (FindedPos != std::string::npos)
				{
					ProcessedColumns.erase(FindedPos, std::string("SELECT").length());
				}

				FindedPos = ProcessedConditionString.find("WHERE");

				if (FindedPos != std::string::npos)
				{
					ProcessedConditionString.erase(FindedPos, std::string("WHERE").length());
				}
				if (ProcessedConditionString.back() == ';')
				{
					ProcessedConditionString.pop_back();
				}

				ResultExec = Query("SELECT " + ProcessedColumns + " FROM " + ProcessedTable +
					" WHERE " + ProcessedConditionString + ";");
			}
			else
			{
				ResultExec = Query("SELECT " + ProcessedColumns + " FROM " + ProcessedTable + ";");
			}

			if (!ResultExec)
			{
				return ReturnJSON;
			}

				// Next Column
			while (ResultExec->next())
			{
				// In This Column (Horizontal, Left-Right Direction)
				sql::ResultSetMetaData *MetaDataResultSet = ResultExec->getMetaData();
				size_t rowsCount = ResultExec->rowsCount(),
					columnCount = MetaDataResultSet->getColumnCount();

				for (size_t i = 1; i <= MetaDataResultSet->getColumnCount(); i++)
				{
					std::string ColumnName, ColumnID = MetaDataResultSet->getColumnLabel(i).asStdString();
					ColumnName.append(ColumnID);

					switch (MetaDataResultSet->getColumnType(i))
					{
						case sql::DataType::BIT:
						case sql::DataType::INTEGER:
						case sql::DataType::NUMERIC:
						case sql::DataType::TINYINT:
						case sql::DataType::SMALLINT:
						case sql::DataType::BIGINT:
						{
							if (rowsCount == 1 && columnCount == 1)
							{
								ReturnJSON = nlohmann::json::object({ { ColumnName, ResultExec->getInt64(ColumnID) } });
							}
							else
							{
								ReturnJSON[ColumnName].push_back(ResultExec->getInt64(ColumnID));
							}

							break;
						}
						case sql::DataType::REAL:
						case sql::DataType::DECIMAL:
						case sql::DataType::DOUBLE:
						{
							if (rowsCount == 1 && columnCount == 1)
							{
								ReturnJSON = nlohmann::json::object({ { ColumnName, ResultExec->getDouble(ColumnID) } });
							}
							else
							{
								ReturnJSON[ColumnName].push_back(ResultExec->getDouble(ColumnID));
							}

							break;
						}
						case sql::DataType::CHAR:
						case sql::DataType::VARCHAR:
						case sql::DataType::LONGVARCHAR:
						case sql::DataType::BINARY:
						case sql::DataType::VARBINARY:
						case sql::DataType::LONGVARBINARY:
						{
							nlohmann::json ParsedJSON;
							std::string stringFromRow = ResultExec->getString(ColumnID).asStdString();

							if (!stringFromRow.empty())
							{
								if (stringFromRow.front() == '\"')
								{
									stringFromRow.erase(stringFromRow.begin());
								}
								if (stringFromRow.back() == '\"')
								{
									stringFromRow.erase(stringFromRow.end());
								}

								ParsedJSON = nlohmann::json::parse(stringFromRow, nullptr, false);

								if (ParsedJSON.is_discarded())
								{
									ParsedJSON = stringFromRow;
								}
							}

							if (rowsCount == 1 && columnCount == 1)
							{
								ReturnJSON = ParsedJSON;
							}
							else
							{
								ReturnJSON[ColumnName].push_back(stringFromRow.empty() ? "" : ParsedJSON);
							}

							break;
						}
					}
				}
			}

			if (ResultExec)
			{
				delete ResultExec;
			}
		}) || Columns.empty())
		{
			return nlohmann::json();
		}

		return ReturnJSON;
	}

	nlohmann::json Client::SelectValuesInCurrentTable(const std::vector<std::string> &Columns,
		const std::vector<std::string> &Condition)
	{
		return SelectValues(CurrentTable, Columns, Condition);
	}

	void Client::UpdateValues(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values, const std::vector<std::string> &condition)
	{
		Exec(query::MakeUpdateValuesQuery(name_table, name_columns, values, condition));
	}

	void Client::UpdateValuesInCurrentTable(const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values, const std::vector<std::string> &condition)
	{
		Exec(query::MakeUpdateValuesQuery(CurrentTable, name_columns, values, condition));
	}

	void Client::InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values)
	{
		Exec(query::MakeInsertValuesQuery(name_table, name_columns, values));
	}

	void Client::InsertValuesInCurrentTable(const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values)
	{
		Exec(query::MakeInsertValuesQuery(CurrentTable, name_columns, values));
	}
	void Client::SetCurrentTable(const std::string &TableName)
	{
		if (!TableName.empty())
		{
			CurrentTable = TableName;
		}
	}
}
