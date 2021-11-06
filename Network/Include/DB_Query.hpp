#include "pch.h"

namespace query
{
	std::string MakeCreateTableQuery (const std::string& name_table, const std::vector<std::string>& name_column, const std::vector<std::string>& type, const std::vector<std::string>& value, const std::vector<std::vector<std::string>>& attributes);
	std::string MakeCreateColumnQuery (const std::string& name_table, const std::string& name_column, const std::string& type, const std::string& value, const std::vector<std::string>& attributes);
	std::string MakeModifyColumnQuery (const std::string& name_table, const std::string& name_column, const std::string& type, const std::string& value, const std::vector<std::string>& attributes);
	std::string MakeDeleteValuesQuery (const std::string& name_table, const std::string& condition);
	std::string MakeDeleteColumnQuery (const std::string& name_table, const std::string& name_column);
	std::string MakeDeleteTableQuery (const std::string& name_table);
	std::string MakeSelectValuesQuery (const std::string& name_table, const std::vector<std::string>& name_columns, const std::vector<std::string>& condition = {});
	std::string MakeUpdateValuesQuery(const std::string& name_table, const std::vector<std::string>& name_columns, const std::vector<std::string>& values, const std::vector<std::string>& condition = {});
	std::string MakeInsertValuesQuery (const std::string& name_table, const std::vector<std::string>& name_columns, const std::vector<std::string>& values);
} // namespace query