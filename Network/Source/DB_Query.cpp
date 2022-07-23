#include "..\Include\DB_Query.hpp"

namespace query
{
	std::string MakeCreateTableQuery(const std::string& name_table, const std::vector<std::string>& name_column,
		const std::vector<std::string>& type, const std::vector<std::string>& value, const std::vector<std::vector<std::string>>& attributes)
	{
		std::string columns = "";

		if (name_column.size() != type.size() || name_column.size() != value.size() || name_column.size() != attributes.size())
		{
#if __has_include("logger.h")
			Logger_Error("Not enough columns data to create table (amount mismatch)!\n");
#endif // HAS_LOGGER

			return "";
		}

		for (size_t col = 0; col < name_column.size(); col++)
		{
			std::string attribute = "";

			for (size_t cnt = 1; cnt < attributes[col].size(); cnt++)
				attribute += attributes[col][cnt-1] + " ";

			if (!attributes[col].empty())
				attribute += attributes[col].back();

			if (type[col] == "TEXT" || value[col].empty())
				columns += "`" + name_column[col] + "` " + type[col] + " " + attribute + ", ";
			else
				columns += "`" + name_column[col] + "` " + type[col] + "(" + value[col] + ")" + attribute + ", ";
		}

		columns.erase(columns.end() - 2, columns.end());

		return ("CREATE TABLE `" + name_table + "`(" + columns + ") DEFAULT CHARSET UTF8;");
	}

	std::string MakeCreateColumnQuery(const std::string& name_table, const std::string& name_column,
		const std::string& type, const std::string& value, const std::vector<std::string>& attributes)
	{
		std::string attribute;

		if (!attributes.empty())
		{
			for (size_t cnt = 0; cnt < attributes.size() - 1; cnt++)
				attribute += attributes[cnt] + " ";

			attribute += attributes.back();
		}

		if (value.empty())
			return ("ALTER TABLE `" + name_table + "`\nADD `" + name_column + "` " + type +
				attribute + ";");
		else
			return ("ALTER TABLE `" + name_table + "`\nADD `" + name_column + "` " + type + "(" + value + ")" +
				attribute + ";");
	}

	std::string MakeModifyColumnQuery(const std::string& name_table, const std::string& name_column,
		const std::string& type, const std::string& value, const std::vector<std::string>& attributes)
	{
		std::string attribute;

		if (!attributes.empty())
		{
			for (size_t cnt = 0; cnt < attributes.size() - 1; cnt++)
				attribute += attributes[cnt] + " ";

			attribute += attributes.back();
		}

		return ("ALTER TABLE `" + name_table + "`\nMODIFY COLUMN `" + name_column + "` " + type + "(" + value + ")" +
			attribute + ";");
	}

	std::string MakeDeleteValuesQuery(const std::string& name_table, const std::string& condition)
	{
		return ("DELETE FROM `" + name_table + "`\nWHERE " + condition + ";");
	}

	std::string MakeDeleteColumnQuery(const std::string& name_table, const std::string& name_column)
	{
		return ("ALTER TABLE `" + name_table + "`\nDROP COLUMN `" + name_column + "`");
	}

	std::string MakeDeleteTableQuery(const std::string& name_table)
	{
		return ("DROP TABLE `" + name_table + "`");
	}

	std::string MakeSelectValuesQuery(const std::string& name_table,
		const std::vector<std::string>& name_columns, const std::vector<std::string>& condition)
	{
		std::string temp;

		if (name_columns.back().back() != '*')
		{
			for (const auto& piece : name_columns)
				temp += "`" + piece + "`,";

			temp.pop_back();
		}
		else
			temp = name_columns.back().back();

		if (!condition.empty())
		{
			std::string NewCond = *condition.data();
			size_t FPos = temp.find("SELECT");
			if (FPos != std::string::npos)
				temp.erase(FPos, strlen("SELECT"));
			FPos = NewCond.find("WHERE");
			if (FPos != std::string::npos)
				NewCond.erase(FPos, strlen("WHERE"));
			if (NewCond.back() == ';')
				NewCond.pop_back();

			return ("SELECT " + temp + " FROM `" + name_table + "` WHERE " + NewCond + ";"); //fix for multiple conditions
		}

		return ("SELECT " + temp + " FROM `" + name_table + "`;");
	}

	std::string MakeUpdateValuesQuery(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& values, const std::vector<std::string>& condition)
	{
		if (name_columns.size() != values.size())
		{
#if __has_include("logger.h")
			Logger_Error("Not enough columns data to create table (amount mismatch)!\n");
#endif // HAS_LOGGER

			return "";
		}

		std::string valueCond, value;
		for (size_t cnt = 0; cnt < name_columns.size(); cnt++)
			value += "`" + name_columns[cnt] + "`='" + values[cnt] + "',";
		std::string Set = value;
		Set.pop_back(); // Removed ','

		for (size_t i = 0; i < condition.size(); i++)
		{
			valueCond += condition.at(i);
		}
		if (!condition.empty())
		{
			std::string NewCond = valueCond;
			size_t FPos = NewCond.find("WHERE");
			if (FPos != std::string::npos)
				NewCond.erase(FPos, strlen("WHERE"));
			if (NewCond.back() == ';')
				NewCond.pop_back();

			return ("UPDATE `" + name_table + "` SET " + Set + " WHERE " + NewCond + ";"); //fix for multiple conditions
		}
		else
			return ("UPDATE `" + name_table + "` SET " + Set);
	}

	std::string MakeInsertValuesQuery(const std::string& name_table, const std::vector<std::string>& name_columns,
		const std::vector<std::string>& values)
	{
		if (name_columns.size() != values.size())
		{
#if __has_include("logger.h")
			Logger_Error("Not enough columns data to create table (amount mismatch)!\n");
#endif // HAS_LOGGER

			return "";
		}

		std::string name_column, temp;

		if (!name_columns.empty())
		{
			for (size_t cnt = 0; cnt < name_columns.size() - 1; cnt++)
				name_column += "`" + name_columns[cnt] + "`, ";

			name_column += "`" + name_columns.back() + "`";
		}

		if (!values.empty())
		{
			temp.insert(0, "(");
			for (size_t i = 0; i < values.size(); i++)
			{
				temp.insert(temp.size(), "'" + values[i] + "',");
			}
			temp.pop_back(); // Remove ','
			temp.push_back(')');

			return ("INSERT INTO `" + name_table + "`(" + name_column + ")" + "VALUES" + temp);
		}
		else
			return ("INSERT INTO `" + name_table + "`() VALUES()");
	}

} //namespace query
