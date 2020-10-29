#include <pch.h>
///////////////////////////////////////
// Headers                           //
///////////////////////////////////////
									 //
#include "MySQL/MySQL_Impl.h"		 //
#include "MySQL/MySQL_Database.h"	 //
									 //
///////////////////////////////////////

using json = nlohmann::json;
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

	void Database::SetNewDatabase(std::shared_ptr<sql::Connection> NewConnection)
	{
		impl = std::make_shared<mysql::Impl>(NewConnection, std::make_shared<Database>());
	}


	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods																				    //
	//////////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::list<std::pair<std::string, json>> Database::SelectValues(const std::string& name_table,			//
		const std::vector<std::string>& name_columns, const std::vector<std::string>& condition)			//
	{
		std::string temp;
		size_t ID = 0;

		if (name_columns.empty())
		{
			throw sql::SQLException("No One Colunms Weren't Selected!");
			return {};
		}

		if (name_columns.back().back() != '*')
		{
			for (const auto &piece: name_columns)
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

			ResultExec = impl->Query("SELECT " + temp + " FROM " + name_table + " WHERE " + NewCond + ";");
		}
		else
			ResultExec = impl->Query("SELECT " + temp + " FROM " + name_table + ";");

		std::list<std::pair<std::string, json>> result;
		if (!ResultExec)
			return result;
		try
		{
			//ResultExec->first();
				// Get Count Columns

				//printf("\nCount Columns: %d", MD->getColumnCount());
				//printf("\nCount Rows: %d", ResultExec->rowsCount());
			while (ResultExec->next())
			{
				json js;
				js["_N"] = ResultExec->getInt("_N");
				js["_0"] = ResultExec->getString("_0");
				js["_1"] = ResultExec->getString("_1");
				js["_2"] = ResultExec->getInt("_2");
				js["_3"] = ResultExec->getInt("_3");
				result.push_back({ std::to_string((int)js["_N"].get<json::value_t>()), js });
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

		if (ResultExec)
			delete ResultExec;
		return result;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Database::UpdateValues(const std::string& name_table, const std::vector<std::string>& name_columns,//
		const std::vector<std::string>& values, const std::vector<std::string>& condition)					//
	{
		std::stringstream valueCond, value;
		for (size_t cnt = 0; cnt < name_columns.size(); cnt++)
			value << name_columns.at(cnt) << "='" << values.at(cnt) << "',";
		std::string Set = value.str();
		Set.pop_back(); // Removed ','

		for (size_t i = 0; i < condition.size(); i++)
		{
			valueCond << condition.at(i);
		}
		if (!condition.empty())
		{
			std::string NewCond = valueCond.str();
			size_t FPos = std::string::npos;
			FPos = NewCond.find("WHERE");
			if (FPos != std::string::npos)
				NewCond.erase(FPos, strlen("WHERE"));
			if (NewCond.back() == ';')
				NewCond.pop_back();

			impl->Exec("UPDATE " + name_table + "\nSET " + Set + "\nWHERE " + NewCond + ";");
		}
		else
			impl->Exec("UPDATE " + name_table + "\nSET " + Set);
	}
} // namespace db
