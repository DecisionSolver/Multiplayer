#include "pch.h"
///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL/MySQL_Database.h"	 //
#include "MySQL/MySQL_Impl.h"		 //
									 //
#include <string>					 //
									 //
///////////////////////////////////////


#if defined(_CONSOLE)
	
	#include <iostream>

#endif // defined(_CONSOLE)

std::map<std::string, std::shared_ptr<mysql::Database>> mysql::Impl::databases;
sql::ConnectOptionsMap mysql::Impl::connection_properties;

namespace mysql
{
	Impl::Status Impl::Connect(const std::string &user, const std::string &password, const std::string &host,
		const std::string &DB, const unsigned short &port, const std::string &charset, bool OnlyRead)
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
	sql::ResultSet *Impl::Query(const std::string &query)
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
				std::cout << "Made Reconnect To Server" << std::endl;

				return Query(query);
			}

			std::cout << "# ERR: SQLException in " << __FILE__;
			std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
			std::cout << "# ERR: " << e.what();
			std::cout << " (MySQL error code: " << e.getErrorCode();
			std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		}

		return nullptr;
	}
	void Impl::Exec(const std::string &query)
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
				std::cout << "Made Reconnect To Server" << std::endl;
				
				Exec(query);
			}
			std::cout << "# ERR: SQLException in " << __FILE__;
			std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
			std::cout << "# ERR: " << e.what();
			std::cout << " (MySQL error code: " << e.getErrorCode();
			std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	void Impl::CreateDatabase(const std::string &name)								  //
	{
		Exec(std::string("CREATE DATABASE " + name).c_str());
	}

	////////////////////////////////////////////////////////////////////////////////////
	void Impl::AddDatabase(const std::string &name)									  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		databases.insert(std::make_pair(name, std::make_shared<Database>()));
		SelectDatabase(name);
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Impl::SelectDatabase(const std::string &name)								  //
	{
		if (!databases.empty())
			current_database = databases.find(name)->second;

		connection->setSchema(name);
		current_database->SetNewDatabase(connection);
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteDatabase(const std::string &name)								  //
	{
		Exec(std::string("DROP DATABASE " + name).c_str());
	}


	////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<Database> Impl::GetCurrentDatabase() const						  //
	{
		return current_database;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::TryInsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,		//
		const std::vector<std::string> &values, const std::vector<std::string> &condition)							//
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");

		if (!wasSelectedDB)
			throw sql::SQLException("Database Was Not Selected!");
		else if (!current_database)
			throw sql::SQLException("Not Ready Current Database!");
		current_database->UpdateValues(name_table, name_columns, values, condition);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	nlohmann::json Impl::TrySelectValues(const std::string &name_table,													 //
		const std::vector<std::string> &name_columns, const std::vector<std::string> &condition)						 //
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
	void Impl::CreateTable(const std::string &name_table, const std::string &name_column,				//
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)	//
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

			Exec("CREATE TABLE " + name_table + "(" + name_column + " " + type + "(" + value + ")" +
				attribute.str() + ") DEFAULT CHARSET utf8;");
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::CreateColumn(const std::string &name_table, const std::string &name_column,				//
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)  //
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
			if(value.empty())
				Exec("ALTER TABLE " + name_table + "\nADD " + name_column + " " + type +
					attribute.str() + ";");
			else
				Exec("ALTER TABLE " + name_table + "\nADD " + name_column + " " + type + "(" + value + ")" +
					attribute.str() + ";");
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::ModifyColumn(const std::string &name_table, const std::string &name_column,				//
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)  //
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

			Exec("ALTER TABLE " + name_table + "\nMODIFY COLUMN " + name_column + " " + type + "(" + value + ")" +
				attribute.str() + ";");
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,			  //
		const std::vector<std::string> &values)																		  //
	{
		if (!connection)
			throw sql::SQLException("Not Connected!");
		else if (isReadOnly)
			throw sql::SQLException("It's Read Only :O !");
		else
			current_database->InsertValues(name_table, name_columns, values);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteValues(const std::string &name_table, const std::string &condition)			  //
	{
		Exec("DELETE FROM " + name_table + "\nWHERE " + condition + ";");
	}


	////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteTable(const std::string &name_table)							  //
	{
		Exec("DROP TABLE " + name_table);
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	void Impl::DeleteColumn(const std::string &name_table, const std::string &name_column)			    //
	{
		Exec("ALTER TABLE " + name_table + "\nDROP COLUMN " + name_column);
	}


	void Impl::Destroy()
	{
		if (connection)
		{
			connection->close();
			connection.reset();
		}
	}
} // namespace mysql
