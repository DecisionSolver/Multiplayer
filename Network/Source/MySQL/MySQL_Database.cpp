
///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL_Impl.h"				 //
#include "MySQL_Database.h"			 //
									 //
///////////////////////////////////////


namespace mysql
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods																				    //
	//////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
	Database::Database()															  //
	{
		impl = std::make_shared<mysql::Impl>();
	}

	void Database::SetNewDatabase(sql::Connection &NewConnection, Database *NewDB)
	{
		impl = std::make_shared<mysql::Impl>(NewConnection, NewDB);
	}


	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods																				    //
	//////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::vector<std::pair<std::string, std::vector<std::string>>> Database::SelectValues(const std::string& name_table,	 //
		const std::vector<std::string>& name_columns, const std::string& condition)										 //
	{
		std::string temp;

		ToDo("Add Condition To Check If Columns Are Present");

		size_t ID = 0;
		for (const auto &piece : name_columns)
		{
			temp += piece + " AS " + "'_" + std::to_string(ID) + "',";
			ID++;
		}
		temp.pop_back();

		sql::ResultSet *ResultExec = nullptr;
		if (!condition.empty())
		{
			std::string NewCond = condition;
			size_t FPos = std::string::npos;
			FPos = temp.find("SELECT");
			if (FPos != std::string::npos)
				temp.erase(FPos, strlen("SELECT"));
			FPos = condition.find("WHERE");
			if (FPos != std::string::npos)
				NewCond.erase(FPos, strlen("WHERE"));

			ResultExec = impl->Query("SELECT " + temp + " FROM " + name_table + " WHERE " + NewCond + ";");
		}
		else
			ResultExec = impl->Query("SELECT " + temp + " FROM " + name_table + ";");

		std::vector<std::pair<std::string, std::vector<std::string>>> result;
		if (!ResultExec)
			return result;
		try
		{
			ResultExec->first();
			while (true)
			{
				sql::ResultSetMetaData *MD;
				MD = ResultExec->getMetaData();
				size_t StartWith = 1;
				// Get Count Columns

				//printf("\nCount Columns: %d", MD->getColumnCount());
				//printf("\nCount Rows: %d", ResultExec->rowsCount());

				for (size_t id_column = 1; id_column < MD->getColumnCount() + 1; id_column++)
				{
					result.push_back({ MD->getColumnLabel(id_column), {} });
					do
					{
						result.back().second.push_back(ResultExec->getString(StartWith));
						StartWith++;
					} while (StartWith < ResultExec->rowsCount() + 1);
				}
				if (!ResultExec->next())
					break;
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

		delete ResultExec;
		return result;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Database::UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,//
		const std::vector<std::string>& values, const std::string& condition)								//
	{
		std::stringstream value;
		std::string temp;

		for (size_t cnt = 0; cnt < name_columns.size(); cnt++)
			value << name_columns.at(cnt) << "='" << values.at(cnt) << "',";

		if (!condition.empty())
		{
			temp = value.str();
			temp.pop_back();

			impl->Query("UPDATE " + name_table + "\nSET " + temp + "\nWHERE " + condition + ";");
		}
		else
		{
			temp = value.str().replace(value.str().size() - 1, 1, ";");
			impl->Query("UPDATE " + name_table + "\nSET " + temp);
		}
	}
} // namespace db
