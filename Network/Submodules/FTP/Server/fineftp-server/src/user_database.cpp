#include "user_database.h"

#include <iostream>

namespace fineftp
{
	UserDatabase::UserDatabase()
	{}

	UserDatabase::~UserDatabase()
	{}

	bool UserDatabase::addUser(const std::string& username, const std::string& password,
		const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);

		auto user_it = database_.find(username);
		if (user_it == database_.end())
		{
			database_.emplace(username, std::shared_ptr<FtpUser>(new FtpUser(new_id_++, md5_from_buffer(password), user_permissions,
				files_permissions)));

			Logger_Info_F("Successfully added user \"%s\".", username.c_str());
			return true;
		}
		else
		{
			Logger_Error_F("Error adding user with username \"%s\". The user already exists.", username.c_str());
			return false;
		}
	}

	std::shared_ptr<FtpUser> UserDatabase::getUser(const std::string& username, const std::string& password) const
	{
		std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);

		auto user_it = database_.find(username);
		if (user_it == database_.end())
		{
			return nullptr;
		}
		else
		{
			if (user_it->second->password_ == md5_from_buffer(password))
				return user_it->second;
			else
				return nullptr;
		}
	}
	
	mysql::Client &UserDatabase::GetClientMysql()
	{
		auto ret = mysql::Client();
		return ret;
	}

	odbc::ODBC &UserDatabase::GetClientODBC()
	{
		auto ret = odbc::ODBC();
		return ret;
	}

	bool UserDatabase::Connect(const std::string& user, const std::string& password, const std::string& host,
		const std::string& DB, const std::string& Table, const unsigned short& port,
		const std::string& charset, bool OnlyRead)
	{
		// To Get Rid Of Much Warnings
		UNREFERENCED_PARAMETER(user);
		UNREFERENCED_PARAMETER(password);
		UNREFERENCED_PARAMETER(host);
		UNREFERENCED_PARAMETER(DB);
		UNREFERENCED_PARAMETER(Table);
		UNREFERENCED_PARAMETER(port);
		UNREFERENCED_PARAMETER(charset);
		UNREFERENCED_PARAMETER(OnlyRead);
		Logger_Error("You have used wrong function on wrong subclass (MySQL variant)");
		return false;
	}

	bool UserDatabase::Connect(const std::string& driver, const std::string& path,
		const std::vector<std::string>& attributes, const std::string& password)
	{
		// To Get Rid Of Much Warnings
		UNREFERENCED_PARAMETER(driver);
		UNREFERENCED_PARAMETER(path);
		UNREFERENCED_PARAMETER(attributes);
		UNREFERENCED_PARAMETER(password);
		Logger_Error("You have used wrong function on wrong subclass (ODBC variant)");
		return false;
	}

	bool MySQLUserDatabase::Connect(const std::string& user, const std::string& password, const std::string& host,
		const std::string& DB, const std::string& Table, const unsigned short& port, const std::string& charset,
		bool OnlyRead)
	{
		if (HaveConnect) return true;
		if (mysqlDB.Connect(user, password, host, DB, Table, port, charset, OnlyRead) != mysql::Client::Status::Done)
		{
			HaveConnect = false;
			return false;
		}
/*
		auto res = mysqlDB.SelectValues(std::vector<std::string>{ "*" });

		for (unsigned int col = 0; col < res["_N"].size(); ++col)
			addUser(res["Username"][col], res["Password"][col], res["UserPermissions"][col],
				res["FilesPermissions"][col]);

		HaveConnect = true;
*/
		return true;
	}

	std::string MySQLUserDatabase::updatePermissions(const std::string& username)
	{
		nlohmann::json res;
		if (database_.find(username) == database_.end())
		{
			res = mysqlDB.SelectValues("local", { "_N", "_1", "_4", "_5" }, { "`_0`='" + username + "'" });
			
			if (res["_0"].empty())
				return "";
			
			std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);

			database_.emplace(username, std::shared_ptr<FtpUser>(new FtpUser(res["_0"][0], res["_1"][0], res["_2"][0], res["_3"][0])));
			
			return username;
		}
		else
		{
			res = mysqlDB.SelectValues("local", { "_N", "_0", "_1", "_4", "_5" }, { "`_N`='" +
			std::to_string(database_[username]->id_) + "'" });

			std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);
			
			if (res["_0"].empty())
			{
				database_.erase(username);
				return "";
			}

			auto user = database_[username];
			if(username != res["_1"][0])
				database_.erase(username);

			user->password_          = res["_2"][0];
			user->user_permissions_  = res["_3"][0];
			user->files_permissions_ = res["_4"][0];

			database_[res["_1"][0]] = user;

			return res["_1"][0];
		}
	}

	mysql::Client &MySQLUserDatabase::GetClientMysql()
	{
		return mysqlDB;
	}

	bool MySQLUserDatabase::addNewUser(const std::string& username, const std::string& password,
		const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		auto res = mysqlDB.SelectValues("local", { "_N" });
		new_id_ = res["_0"] + 1ull;

		if (!addUser(username, password, user_permissions, files_permissions))
			return false;

		mysqlDB.InsertValues("local", { "_0", "_1", "_4", "_5" }, { username, md5_from_buffer(password),
		std::to_string((int)user_permissions), files_permissions.dump() });
		return true;
	}

	bool ODBCUserDatabase::Connect(const std::string& driver, const std::string& path,
		const std::vector<std::string>& attributes, const std::string& password)
	{
		if (HaveConnect) return true;
		if (!odbcDB.Connect(driver, path, attributes, password))
		{
			HaveConnect = false;
			return false;
		}

/*
		auto res = odbcDB.SelectValues("user_wright", { "*" });

		for (unsigned int col = 0; col < res["_N"].size(); ++col)
			addUser(res["Username"][col], res["Password"][col], res["UserPermissions"][col], res["FilesPermissions"][col]);
*/
		HaveConnect = true;
		return true;
	}

	std::string ODBCUserDatabase::updatePermissions(const std::string& username)
	{
		nlohmann::json res;
		if (database_.find(username) == database_.end())
		{
			res = odbcDB.SelectValues("local", { "_N", "_1", "_4", "_5" }, { "`_0`='" + username + "'" });

			if (res["_0"].empty()) 
				return "";
			
			std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);

			database_.emplace(username, std::shared_ptr<FtpUser>(new FtpUser(res["_N"][0], res["_1"][0],
			res["_4"][0], res["_5"][0])));

			return username;
		}
		else
		{
			res = odbcDB.SelectValues("local", { "_N", "_0", "_1", "_4", "_5" }, { "`_N`='" +
			std::to_string(database_[username]->id_) + "'" });

			std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);

			if (res["_0"].empty())
			{
				database_.erase(username);
				return "";
			}

			auto user = database_[username];
			database_.erase(username);

			user->password_ = res["_1"][0];
			user->user_permissions_ = res["_3"][0];
			user->files_permissions_ = res["_4"][0];

			database_[res["_1"][0]] = user;

			return res["_1"][0];
		}
	}

	odbc::ODBC &ODBCUserDatabase::GetClientODBC()
	{
		return odbcDB;
	}

	bool ODBCUserDatabase::addNewUser(const std::string& username, const std::string& password,
	const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		auto res = odbcDB.SelectValues("local", { "_N" });
		new_id_ = res["_0"] + 1ull;

		if (!addUser(username, password, user_permissions, files_permissions))
			return false;

		odbcDB.InsertValues("local", { "_0", "_1", "_4", "_5" }, { username, md5_from_buffer(password),
		std::to_string((int)user_permissions), files_permissions.dump() });
		return true;
	}
}