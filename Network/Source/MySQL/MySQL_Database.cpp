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
	json Database::SelectValues(const std::string &name_table,												//
		const std::vector<std::string> &name_columns, const std::vector<std::string> &condition)			//
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

		json js = {};
		if (!ResultExec)
			return js;
		try
		{
			while (ResultExec->next())
			{
				auto MetaData = ResultExec->getMetaData();
				for (size_t i = 1; i <= MetaData->getColumnCount(); i++)
				{
					int colType = MetaData->getColumnType(i);
					std::string ColumnName = MetaData->getColumnLabel(i);

					switch (colType)
					{
					case sql::DataType::BIT:
					case sql::DataType::INTEGER:
					case sql::DataType::NUMERIC:
					case sql::DataType::TINYINT:
					case sql::DataType::SMALLINT:
					case sql::DataType::BIGINT:
						js[ColumnName].push_back(ResultExec->getInt64(ColumnName));
						break;
					case sql::DataType::REAL:
					case sql::DataType::DECIMAL:
					case sql::DataType::DOUBLE:
						js[ColumnName].push_back(ResultExec->getDouble(ColumnName));
						break;
					case sql::DataType::CHAR:
					case sql::DataType::VARCHAR:
					case sql::DataType::LONGVARCHAR:
					case sql::DataType::BINARY:
					case sql::DataType::VARBINARY:
					case sql::DataType::LONGVARBINARY:
					{
						std::string str = ResultExec->getString(ColumnName).c_str();
						json _js = json::parse(str);
						if (!str.empty() && !_js.empty() && _js.is_object())
						{
							for (auto&[key, val]: _js.items())
							{
								for (auto&[_, elm]: val.items())
								{
									js[key].push_back(elm);
								}
							}
						}
						else if (_js.is_array())
							js[ColumnName].push_back(json({_js})[0]);
						else
							js[ColumnName].push_back(str.empty() ? "" : _js);
						break;
					}
					}
				}
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
		return js;
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
