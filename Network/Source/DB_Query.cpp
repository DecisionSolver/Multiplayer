#include "..\Include\DB_Query.hpp"

namespace query
{
	std::string MakeCreateTableQuery(const std::string &name_table, const std::vector<std::string> &name_column,
		const std::vector<std::string> &type, const std::vector<std::string> &value,
		const std::vector<std::vector<std::string>> &attributes)
	{
		if (name_column.size() != type.size() || name_column.size() != value.size() || name_column.size() != attributes.size())
		{
#if __has_include("logger.h")
			Logger_Error("Not enough columns data to create table (amount mismatch)!\n");
#endif // HAS_LOGGER

			return "";
		}

		std::string columns;
		for (size_t ColumnIndex = 0; ColumnIndex < name_column.size(); ColumnIndex++)
		{
			std::string attribute = "";

			for (size_t cnt = 1; cnt < attributes[ColumnIndex].size(); cnt++)
			{
				attribute += attributes[ColumnIndex][cnt - 1] + " ";
			}

			if (!attributes[ColumnIndex].empty())
			{
				attribute += attributes[ColumnIndex].back();
			}

			if (type[ColumnIndex] == "TEXT" || value[ColumnIndex].empty())
			{
				columns += "`" + name_column[ColumnIndex] + "` " + type[ColumnIndex] + " " + attribute + ", ";
			}
			else
			{
				columns += "`" + name_column[ColumnIndex] + "` " + type[ColumnIndex] + "(" + value[ColumnIndex] + ")" +
					attribute + ", ";
			}
		}

		columns.erase(columns.end() - 2, columns.end());

		return ("CREATE TABLE `" + name_table + "`(" + columns + ") DEFAULT CHARSET UTF8;");
	}

	std::string MakeCreateColumnQuery(const std::string &name_table, const std::string &name_column,
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)
	{
		std::string attribute;

		if (!attributes.empty())
		{
			for (size_t AttributeIndex = 0; AttributeIndex < attributes.size() - 1; AttributeIndex++)
			{
				attribute += attributes[AttributeIndex] + " ";
			}

			attribute += attributes.back();
		}

		if (value.empty())
		{
			return ("ALTER TABLE `" + name_table + "`\nADD `" + name_column + "` " + type +
				attribute + ";");
		}
		else
		{
			return ("ALTER TABLE `" + name_table + "`\nADD `" + name_column + "` " + type + "(" + value + ")" +
				attribute + ";");
		}

		return "";
	}

	std::string MakeModifyColumnQuery(const std::string &name_table, const std::string &name_column,
		const std::string &type, const std::string &value, const std::vector<std::string> &attributes)
	{
		std::string attribute;

		if (!attributes.empty())
		{
			for (size_t AttributeIndex = 0; AttributeIndex < attributes.size() - 1; AttributeIndex++)
			{
				attribute += attributes[AttributeIndex] + " ";
			}

			attribute += attributes.back();
		}

		return ("ALTER TABLE `" + name_table + "`\nMODIFY COLUMN `" + name_column + "` " + type + "(" + value + ")" +
			attribute + ";");
	}

	std::string MakeDeleteValuesQuery(const std::string &name_table, const std::string &condition)
	{
		return ("DELETE FROM `" + name_table + "`\nWHERE " + condition + ";");
	}

	std::string MakeDeleteColumnQuery(const std::string &name_table, const std::string &name_column)
	{
		return ("ALTER TABLE `" + name_table + "`\nDROP COLUMN `" + name_column + "`");
	}

	std::string MakeDeleteTableQuery(const std::string &name_table)
	{
		return ("DROP TABLE `" + name_table + "`");
	}

	std::string MakeSelectValuesQuery(const std::string &name_table,
		const std::vector<std::string> &name_columns, const std::vector<std::string> &condition)
	{
		std::string Columns;

		if (name_columns.back().back() != '*')
		{
			for (const auto &column : name_columns)
			{
				Columns += "`" + column + "`,";
			}

			Columns.pop_back();
		}
		else
		{
			Columns = name_columns.back().back();
		}

		if (!condition.empty())
		{
			std::string Condition = *condition.data();
			size_t FindPosition = Columns.find("SELECT");
			if (FindPosition != std::string::npos)
			{
				Columns.erase(FindPosition, strlen("SELECT"));
			}
			
			FindPosition = Condition.find("WHERE");
			if (FindPosition != std::string::npos)
			{
				Condition.erase(FindPosition, strlen("WHERE"));
			}
			if (Condition.back() == ';')
			{
				Condition.pop_back();
			}

			return ("SELECT " + Columns + " FROM `" + name_table + "` WHERE " + Condition
				+ ";"); //fix for multiple conditions
		}

		return ("SELECT " + Columns + " FROM `" + name_table + "`;");
	}

	std::string MakeUpdateValuesQuery(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values, const std::vector<std::string> &condition)
	{
		if (name_columns.size() != values.size())
		{
#if __has_include("logger.h")
			Logger_Error("Not enough columns data to create table (amount mismatch)!\n");
#endif
		}
		else
		{
			std::string Set_Columns;
			for (size_t ColumnIndex = 0; ColumnIndex < name_columns.size(); ColumnIndex++)
			{
				Set_Columns += "`" + name_columns[ColumnIndex] + "`='" + values[ColumnIndex] + "',";
			}
			Set_Columns.pop_back(); // Remove ','

			if (!condition.empty())
			{
				std::string Condition;
				size_t FoundPosition = Condition.find("WHERE");
				
				for (size_t i = 0; i < condition.size(); i++)
				{
					Condition += condition.at(i);
				}

				if (FoundPosition != std::string::npos)
				{
					Condition.erase(FoundPosition, strlen("WHERE"));
				}
				if (Condition.back() == ';')
				{
					Condition.pop_back();
				}

				return ("UPDATE `" + name_table + "` SET " + Set_Columns + " WHERE " + Condition + ";"); //fix for multiple conditions
			}
			else
			{
				return ("UPDATE `" + name_table + "` SET " + Set_Columns);
			}
		}

		return "";
	}

	std::string MakeInsertValuesQuery(const std::string &name_table, const std::vector<std::string> &name_columns,
		const std::vector<std::string> &values)
	{
		if (name_columns.size() != values.size())
		{
#if __has_include("logger.h")
			Logger_Error("Not enough columns data to create table (amount mismatch)!\n");
#endif
		}
		else if (!name_columns.empty() && !values.empty())
		{
			std::string Column, Values;

			for (size_t ColumnIndex = 0; ColumnIndex < name_columns.size() - 1; ColumnIndex++)
			{
				Column += "`" + name_columns[ColumnIndex] + "`, ";
			}

			Column += "`" + name_columns.back() + "`";

			Values.insert(0, "(");
			for (size_t i = 0; i < values.size(); i++)
			{
				Values.insert(Values.size(), "'" + values[i] + "',");
			}
			Values.pop_back(); // Remove ','
			Values.push_back(')');

			return ("INSERT INTO `" + name_table + "`(" + Column + ")" + "VALUES" + Values);
		}
		else
		{
			return ("INSERT INTO `" + name_table + "`() VALUES()");
		}

		return "";
	}
	std::string MakeCreateDatabaseQuery(const std::string &name_database)
	{
		if (!name_database.empty())
		{
			return ("CREATE DATABASE IF NOT EXISTS `" + name_database + "`;");
		}

		return "";
	}
	std::string MakeDropDatabaseQuery(const std::string &name_database)
	{
		if (!name_database.empty())
		{
			return ("DROP DATABASE `" + name_database + "`");
		}

		return "";
	}
}
