///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL_Database.h"			 //
#include "MySQL_Impl.h"				 //
									 //
#include <string>					 //
									 //
///////////////////////////////////////


#if defined(_CONSOLE)
	
	#include <iostream>

#endif // defined(_CONSOLE)

std::map<std::string, std::shared_ptr<mysql::Database>> mysql::Impl::databases;

namespace mysql
{
	Impl::Status Impl::Connect(const std::string & user, const std::string & password, const std::string & host,
		const std::string DB, const unsigned short & port, const std::string charset, bool OnlyRead)
	{
		try
		{
			sql::ConnectOptionsMap connection_properties;

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

			if (!DB.empty())
				AddDatabase(DB);
			return Impl::Status::Done;
		}
		catch (sql::SQLException &e)
		{
			std::cout << "# ERR: SQLException in " << __FILE__;
			std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
			std::cout << "# ERR: " << e.what();
			std::cout << " (MySQL error code: " << e.getErrorCode();
			std::cout << ", SQLState: " << e.getSQLState() <<
				" )" << std::endl;
			return Impl::Status::Error;
		}
		return Impl::Status::Error;
	}
	sql::ResultSet *Impl::Query(const std::string& query)
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
			std::cout << "# ERR: SQLException in " << __FILE__;
			std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
			std::cout << "# ERR: " << e.what();
			std::cout << " (MySQL error code: " << e.getErrorCode();
			std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		}

		return nullptr;
	}

	////////////////////////////////////////////////////////////////////////////////////
	void Impl::CreateDatabase(const std::string& name)								  //
	{
		Query(std::string("CREATE DATABASE " + name).c_str());
	}

	////////////////////////////////////////////////////////////////////////////////////
	void Impl::AddDatabase(const std::string& name)									  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		databases.insert(std::make_pair(name, std::make_shared<Database>()));
		SelectDatabase(name);
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Impl::SelectDatabase(const std::string& name)								  //
	{
		if (!databases.empty())
			current_database = databases.find(name)->second;

		connection->setSchema(name);
		//if (!current_database)
		//{
		//	current_database = new Database();
		current_database->SetNewDatabase(connection);
		//}
		//else
		//	current_database->SetNewDatabase(connection, current_database);
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteDatabase(const std::string& name)								  //
	{
		Query(std::string("DROP DATABASE " + name).c_str());
	}


	////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<Database> Impl::GetCurrentDatabase() const						  //
	{
		return current_database;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::TryInsertValues(const std::string & name_table, const std::vector<std::string>& name_columns,		//
		const std::vector<std::string>& values)																		//
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		ToDo("A Request To Server About Inserting New Values!");
		std::vector<std::string> value;
		std::string values_string;
		size_t iV = 0;
		if (!values.empty())
		{
			for (size_t iC = 0; iC < name_columns.size(); iC++)
			{
				for (; iV < values.size();)
				{
					value.push_back(name_columns.at(iC));
					if (isReadOnly)
						value.back() += " = " + ("'" + values.at(iV) + "'") + ",";
					else
						value.front() += "(" + ("'" + values.at(iV) + "'") + ",";
					if (!isReadOnly)
					{
						value.back().push_back(')');
						value.back().push_back(',');
					}
					iV++;
					break;
				}
			}
			values_string = std::accumulate(value.begin(), value.end(), std::string{});
			values_string.pop_back();
			if (isReadOnly)
				Query("UPDATE " + name_table + " SET " + values_string + ";");
			else
			{
				std::string which_columns, what_values;
				for (const auto &piece : name_columns) which_columns += piece + ",";
				for (const auto &piece : values) what_values += piece + ",";
				Query("INSERT " + name_table + "(" + which_columns + ")" + " VALUES" + "(" + what_values + ")" + ";");
			}
		}
		else
			throw sql::SQLException("Nothing Insert!");
		//else if (!isReadOnly)
		//	Query("INSERT " + name_table + "() VALUES()");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::vector<std::pair<std::string, std::vector<std::string>>> Impl::TrySelectValues(const std::string & name_table,	 //
		const std::vector<std::string>& name_columns, const std::string & condition)									 //
	{
		if (!wasSelectedDB)
			throw sql::SQLException("Database Was Not Selected!");
		else if (!current_database)
			throw sql::SQLException("Not Ready Current Database!");
		return current_database->SelectValues(name_table, name_columns, condition);
	}


	////////////////////////////////////////////////////////////////////////////////////
	const std::vector<std::string> Impl::GetDatabaseNames() const					  //
	{
		std::vector<std::string> _names;

		for (auto& item : databases)
			_names.push_back(item.first);

		return _names;
	}


	////////////////////////////////////////////////////////////////////////////////////
	size_t Impl::GetSizeDatabases() const											  //
	{
		return databases.size();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::CreateTable(const std::string& name_table, const std::string& name_column,				//
		const std::string& type, const std::string& value, const std::vector<std::string>& attributes)	//
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
		{
			std::stringstream attribute;

			for (size_t cnt = 0; cnt < attributes.size() - 1; cnt++)
				attribute << attributes.at(cnt) << " ";

			attribute << attributes.back();

			Query("CREATE TABLE " + name_table + "(" + name_column + " " + type + "(" + value + ")" +
				attribute.str() + ") DEFAULT CHARSET utf8;");
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::CreateColumn(const std::string& name_table, const std::string& name_column,				//
		const std::string& type, const std::string& value, const std::vector<std::string>& attributes)  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
		{
			std::stringstream attribute;

			if (!attributes.empty())
			{
				for (size_t cnt = 0; cnt < attributes.size() - 1; cnt++)
					attribute << attributes.at(cnt) << " ";

				attribute << attributes.back();
			}

			Query("ALTER TABLE " + name_table + "\nADD " + name_column + " " + type + "(" + value + ")" +
				attribute.str() + ";");
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::ModifyColumn(const std::string& name_table, const std::string& name_column,				//
		const std::string& type, const std::string& value, const std::vector<std::string>& attributes)  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
		{
			std::stringstream attribute;

			if (!attributes.empty())
			{
				for (size_t cnt = 0; cnt < attributes.size() - 1; cnt++)
					attribute << attributes.at(cnt) << " ";

				attribute << attributes.back();
			}

			Query("ALTER TABLE " + name_table + "\nMODIFY COLUMN " + name_column + " " + type + "(" + value + ")" +
				attribute.str() + ";");
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::InsertValues(const std::string& name_table, const std::vector<std::string>& name_columns,			  //
		const std::vector<std::vector<std::string>>& values)														  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
		{
			std::stringstream name_column, value;
			std::string temp[2];

			if (!name_columns.empty())
			{
				for (size_t cnt = 0; cnt < name_columns.size() - 1; cnt++)
					name_column << name_columns.at(cnt) + ", ";

				name_column << name_columns.back();
			}

			if (!values.empty())
			{
				for (size_t i = 0; i < values.size(); i++)
				{
					value << "(";
					for (size_t j = 0; j < values.at(i).size() - 1; j++)
						value << "'" << values.at(i).at(j) + "',";

					value << "'" << values.at(i).back() << "'),";
				}

				temp[0] = name_column.str();
				temp[1] = value.str().replace(value.str().back(), 1, ";");

				Query("INSERT " + name_table + "(" + temp[0] + ")" + "VALUES" + temp[1]);
			}
			else
				Query("INSERT " + name_table + "() VALUES()");
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteValues(const std::string& name_table, const std::string& condition)			  //
	{
		Query("DELETE FROM " + name_table + "\nWHERE " + condition + ";");
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteTable(const std::string& name_table)							  //
	{
		Query("DROP TABLE " + name_table);
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteColumn(const std::string& name_table, const std::string& name_column)			    //
	{
		Query("ALTER TABLE " + name_table + "\nDROP COLUMN " + name_column);
	}


	void Impl::Destroy()
	{
		//delete stmt;
	}
} // namespace mysql
