#include <pch.h>
///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL/MySQL_Client.h"		 //
#include "SQL_Query.hpp"			 //
									 //
///////////////////////////////////////

#if defined (_DEBUG)
ToDo("Add Method For Change CharSet (UTF)")
#endif

sql::ConnectOptionsMap mysql::Client::connection_properties;

namespace mysql
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods																					//
	//////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
	void Client::Disconnect()														  //
	{
		DebugBreak();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Client::Status Client::Connect(const std::string &user, const std::string &password, const std::string &host,	//
		const std::string &DB, const unsigned short &port, const std::string &charset, bool OnlyRead)				//
	{
		try
		{
			connection_properties["hostName"] = host;
			connection_properties["userName"] = user;
			connection_properties["password"] = password;
			if (!DB.empty())
			{
				connection_properties["schema"] = DB;
				wasSelectedDB = true;
			}
			else
			{
				wasSelectedDB = false;
				throw sql::SQLException("Database Was Not Selected!");
			}
			connection_properties["port"] = port;
			connection_properties["OPT_RECONNECT"] = true;
			if (!charset.empty())
				connection_properties["OPT_CHARSET_NAME"] = charset;

			driver = get_driver_instance();
			connection.reset(driver->connect(connection_properties));

			if (OnlyRead)
				isReadOnly = true;
			return Client::Status::Done;
		}
		catch (sql::SQLException &e)
		{
#if defined(HAS_LOGGER)
			Logger_Critical_F("SQLException:\n%s\nCode: %i,\nSQLState: %s", e.what(), e.getErrorCode(), e.getSQLState().c_str());
#endif
			return Client::Status::Error;
		}
	}
	
	
	//////////////////////////////////////////////////////////////////////////////////////
	sql::ResultSet *Client::Query(const std::string &query)								//
	{
		try
		{
			if (!connection)
				throw sql::SQLException("Not Connected!");
			if (!wasSelectedDB)
				throw sql::SQLException("Database Was Not Selected!");
			else
			{
				std::string NewQuery = query;
				if (NewQuery.back() != ';')
					NewQuery.push_back(';');
				else if (std::count(NewQuery.begin(), NewQuery.end(), ';') > 2)
					NewQuery.erase(NewQuery.find(';', 1));

				sql::Statement *stmt = nullptr;
				stmt = connection->createStatement();
				return stmt->executeQuery(NewQuery);
			}
		}
		catch (sql::SQLException &e)
		{
			// If "Lost Connection To MySQL Server During Query"
			if (e.getErrorCode() == 2013)
			{
				driver = get_driver_instance();
				connection.reset(driver->connect(connection_properties));

#if defined(HAS_LOGGER)
				Logger_Info("Made Reconnect To Server");
#endif
				return Query(query);
			}
#if defined(HAS_LOGGER)
			Logger_Critical_F("SQLException:\n%s\nCode: %i,\nSQLState: %s", e.what(), e.getErrorCode(), e.getSQLState().c_str());
#endif
		}

		return nullptr;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////////////
	void Client::Exec(const std::string &query)											//
	{
		try
		{
			if (!connection)
				throw sql::SQLException("Not Connected!");
			if (!wasSelectedDB)
				throw sql::SQLException("Database Was Not Selected!");
			else
			{
				std::string NewQuery = query;
				if (NewQuery.back() != ';')
					NewQuery.push_back(';');
				else if (std::count(NewQuery.begin(), NewQuery.end(), ';') > 2)
					NewQuery.erase(NewQuery.find(';', 1));

				sql::Statement *stmt = nullptr;
				stmt = connection->createStatement();
				stmt->execute(NewQuery);
			}
		}
		catch (sql::SQLException &e)
		{
			// If "Lost Connection To MySQL Server During Query"
			if (e.getErrorCode() == 2013)
			{
				driver = get_driver_instance();
				connection.reset(driver->connect(connection_properties));

#if defined(HAS_LOGGER)
				Logger_Info("Made Reconnect To Server");
#endif
				Exec(query);
			}
#if defined(HAS_LOGGER)
			Logger_Critical_F("SQLException:\n%s\nCode: %i,\nSQLState: %s", e.what(), e.getErrorCode(), e.getSQLState().c_str());
#endif
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	void Client::CreateDatabase(const std::string &name)							  //
	{
		Exec(std::string("CREATE DATABASE " + name).c_str());
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Client::DeleteDatabase(const std::string &name)							  //
	{
		Exec(std::string("DROP DATABASE " + name).c_str());
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::CreateTable(const std::string &name_table, const std::string &name_column,				//
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)	//
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
			Exec(query::MakeCreateTableQuery(name_table, name_column, type, value, attributes));
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::CreateColumn(const std::string &name_table, const std::string &name_column,			//
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
			Exec(query::MakeCreateColumnQuery(name_table, name_column, type, value, attributes));
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::ModifyColumn(const std::string &name_table, const std::string &name_column,			//
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
			Exec(query::MakeModifyColumnQuery(name_table, name_column, type, value, attributes));
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::DeleteValues(const std::string &name_table, const std::string &condition)			  //
	{
		Exec(query::MakeDeleteValuesQuery(name_table, condition));
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Client::DeleteTable(const std::string &name_table)							  //
	{
		Exec(query::MakeDeleteTableQuery(name_table));
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::DeleteColumn(const std::string &name_table, const std::string &name_column)			//
	{
		Exec(query::MakeDeleteColumnQuery(name_table, name_column));
	}


	//////////////////////////////
	void Client::Destroy()		//
	{
		if (connection)
		{
			connection->close();
			connection.reset();
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	nlohmann::json Client::TrySelectValues(const std::string &name_table,									//
		const std::vector<std::string> &name_columns, const std::vector<std::string> &condition)			//
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		if (!wasSelectedDB)
			throw sql::SQLException("Database Was Not Selected!");

		std::string temp;
		size_t ID = 0;

		if (name_columns.empty())
			throw sql::SQLException("No One Colunms Weren't Selected!");

		if (name_columns.back().back() != '*')
		{
			for (const auto &piece : name_columns)
			{
				temp += piece + " AS " + "'_" + std::to_string(ID) + "',";
				ID++;
			}
			temp.pop_back();
		}
		else
			temp = name_columns.back().back();

		sql::ResultSet *ResultExec = nullptr;
		if (!condition.empty())
		{
			std::string NewCond = *condition.data();
			size_t FPos = std::string::npos;
			FPos = temp.find("SELECT");
			if (FPos != std::string::npos)
				temp.erase(FPos, strlen("SELECT"));
			FPos = NewCond.find("WHERE");
			if (FPos != std::string::npos)
				NewCond.erase(FPos, strlen("WHERE"));
			if (NewCond.back() == ';')
				NewCond.pop_back();

			ResultExec = Query("SELECT " + temp + " FROM " + name_table + " WHERE " + NewCond + ";");
		}
		else
			ResultExec = Query("SELECT " + temp + " FROM " + name_table + ";");

		json js = {};
		if (!ResultExec)
			return js;
		try
		{
			// Next Column
			while (ResultExec->next())
			{
				// In This Column (Horizontal, Left-Right Direction)
				auto MetaData = ResultExec->getMetaData();
				for (size_t i = 1; i <= MetaData->getColumnCount(); i++)
				{
					int colType = MetaData->getColumnType(i);
					std::string ColumnName, /*"_" + std::to_string(ID),*/ ColumnID = MetaData->getColumnLabel(i);
					
					ColumnName = ColumnID;

					switch (colType)
					{
					case sql::DataType::BIT:
					case sql::DataType::INTEGER:
					case sql::DataType::NUMERIC:
					case sql::DataType::TINYINT:
					case sql::DataType::SMALLINT:
					case sql::DataType::BIGINT:
						js[ColumnName].push_back(ResultExec->getInt64(ColumnID));
						break;
					case sql::DataType::REAL:
					case sql::DataType::DECIMAL:
					case sql::DataType::DOUBLE:
						js[ColumnName].push_back(ResultExec->getDouble(ColumnID));
						break;
					case sql::DataType::CHAR:
					case sql::DataType::VARCHAR:
					case sql::DataType::LONGVARCHAR:
					case sql::DataType::BINARY:
					case sql::DataType::VARBINARY:
					case sql::DataType::LONGVARBINARY:
					{
						std::string str = ResultExec->getString(ColumnID).c_str();

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
								for (auto&[key, val] : _js.items())
								{
									for (auto&[_, elm] : val.items())
									{
										js[key].push_back(elm);
									}
								}
								break;
							}
							else if (_js.is_array())
							{
								js[ColumnName].push_back(json({ _js })[0]);
								break;
							}
						}
						js[ColumnName].push_back(str.empty() ? "" : _js);
						break;
					}
					}

					ID++;
				}
			}
		}
		catch (sql::SQLException &e)
		{
#if defined(HAS_LOGGER)
		Logger_Critical_F("SQLException:\n%s\nCode: %i,\nSQLState: %s", e.what(), e.getErrorCode(), e.getSQLState().c_str());
#endif
		}

		if (ResultExec)
			delete ResultExec;
		return js;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::TryUpdateValues(const std::string &name_table, const std::vector<std::string> &name_columns, //
		const std::vector<std::string> &values, const std::vector<std::string> &condition)					  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		if (!wasSelectedDB)
			throw sql::SQLException("Database Was Not Selected!");

		Exec(query::MakeUpdateValuesQuery(name_table, name_columns, values, condition));
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Client::TryInsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,	//
		const std::vector<std::string> &values)																	//
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		if (!wasSelectedDB)
			throw sql::SQLException("Database Was Not Selected!");

		Exec(query::MakeInsertValuesQuery(name_table, name_columns, values));
	}
} // namespace mysql
