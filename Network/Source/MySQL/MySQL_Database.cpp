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
	std::list<std::pair<std::string, json>> Database::SelectValues(const std::string &name_table,			//
		const std::vector<std::string> &name_columns, const std::vector<std::string> &condition)			//
	{
		std::string temp;

		if (name_columns.empty())
		{
			throw sql::SQLException("No One Colunms Weren't Selected!");
			return {};
		}

		if (name_columns.back().back() != '*')
		{
			for (const auto &piece: name_columns)
			{
				temp += piece + ",";
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
			while (ResultExec->next())
			{
				json js;
				if (ResultExec->findColumn("_N") > 0)
					js["_N"] = ResultExec->getInt("_N");
				if (ResultExec->findColumn("_0") > 0)
				{
					js["_0"] = ResultExec->getString("_0");
					if ((js["_0"].is_string() && !js["_0"].get<json::string_t>().empty() &&
						(js["_0"].dump().find("[") != std::string::npos &&
							js["_0"].dump().rfind("]") != std::string::npos)))
						js["_0"] = json::parse(js["_0"].get<json::string_t>());
				}
				if (ResultExec->findColumn("_1") > 0)
				{
					js["_1"] = ResultExec->getString("_1");
					if ((js["_1"].is_string() && !js["_1"].get<json::string_t>().empty() &&
						(js["_1"].dump().find("[") != std::string::npos &&
							js["_1"].dump().rfind("]") != std::string::npos)))
						js["_1"] = json::parse(js["_1"].get<json::string_t>());
				}
				if (ResultExec->findColumn("_2") > 0)
					js["_2"] = ResultExec->getInt("_2");
				if (ResultExec->findColumn("_3") > 0)
					js["_3"] = ResultExec->getInt("_3");

				if (js.find("_N") != js.end())
					result.push_back({
					 (js["_N"].is_number() ?
						std::to_string((int)js["_N"].get<json::value_t>()) :
						js["_N"].get<json::string_t>()),
						js });
				else
					result.push_back({
						(js.front().is_number() ?
						std::to_string((int)js.front().get<json::value_t>()) :
						js.front().get<json::string_t>()),
						js });
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
	void Database::UpdateValues(const std::string &name_table, const std::vector<std::string> &name_columns,//
		const std::vector<std::string> &values, const std::vector<std::string> &condition)					//
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

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Database::InsertValues(const std::string &name_table, const std::vector<std::string> &name_columns,	//
		const std::vector<std::string> &values)																	//
	{
		std::stringstream name_column;
		std::string temp;

		if (!name_columns.empty())
		{
			for (size_t cnt = 0; cnt < name_columns.size() - 1; cnt++)
				name_column << name_columns.at(cnt) + ", ";

			name_column << name_columns.back();
		}

		if (!values.empty())
		{
			temp.insert(0, "(");
			for (size_t i = 0; i < values.size(); i++)
			{
				temp.insert(temp.size(), "'" + values.at(i) + "',");
			}
			temp.pop_back(); // Remove ','
			temp.push_back(')');


			impl->Exec("INSERT " + name_table + "(" + name_column.str() + ")" + "VALUES" + temp);
		}
		else
			impl->Exec("INSERT " + name_table + "() VALUES()");
	}
} // namespace db
