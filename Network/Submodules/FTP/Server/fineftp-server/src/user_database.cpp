#include "user_database.h"

#include <iostream>


namespace fineftp
{
	UserDatabase::UserDatabase()
	{}

	UserDatabase::~UserDatabase()
	{}

	bool UserDatabase::addUser(const std::string& username, const std::string& password, const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		std::lock_guard<decltype(database_mutex_)> database_lock(database_mutex_);

		auto user_it = database_.find(username);
		if (user_it == database_.end())
		{
			database_.emplace(username, std::shared_ptr<FtpUser>(new FtpUser(new_id_++, md5_from_buffer(password), user_permissions, files_permissions)));

#ifndef NDEBUG
			std::cout << "Successfully added user \"" << username << "\"." << std::endl;
#endif // !NDEBUG
			return true;
		}
		else
		{
			std::cerr << "Error adding user with username \"" << username << "\". The user already exists." << std::endl;
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

	bool UserDatabase::Connect(const std::string& user, const std::string& password, const std::string& host, const std::string& DB, const unsigned short& port, const std::string& charset, bool OnlyRead)
	{
		std::cerr << "You have used wrong function on wrong subclass (MySQL variant)\n";
		return false;
	}

	bool UserDatabase::Connect(const std::string& driver, const std::string& path, const std::vector<std::string>& attributes, const std::string& password)
	{
		std::cerr << "You have used wrong function on wrong subclass (ODBC variant)\n";
		return false;
	}

	bool MySQLUserDatabase::Connect(const std::string& user, const std::string& password, const std::string& host, const std::string& DB, const unsigned short& port, const std::string& charset, bool OnlyRead)
	{
		if (mysqlDB.Connect(user, password, host, DB, port, charset, OnlyRead) != mysql::Client::Status::Done)
			return false;

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
			res = mysqlDB.SelectValues("local", { "_N", "_0", "_1", "_4", "_5" }, { "`_N`='" + std::to_string(database_[username]->id_) + "'" });

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

	bool MySQLUserDatabase::addNewUser(const std::string& username, const std::string& password, const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		auto res = mysqlDB.SelectValues("local", { "_N" });
		new_id_ = res["_0"] + 1ull;

		if (!addUser(username, password, user_permissions, files_permissions))
			return false;

		mysqlDB.InsertValues("local", { "_0", "_1", "_4", "_5" }, { username, md5_from_buffer(password), std::to_string((int)user_permissions), files_permissions.dump() });
		return true;
	}

	bool ODBCUserDatabase::Connect(const std::string& driver, const std::string& path, const std::vector<std::string>& attributes, const std::string& password)
	{
		if (!odbcDB.Connect(driver, path, attributes, password))
			return false;

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

			database_.emplace(username, std::shared_ptr<FtpUser>(new FtpUser(res["_N"][0], res["_1"][0], res["_4"][0], res["_5"][0])));

			return username;
		}
		else
		{
			res = odbcDB.SelectValues("local", { "_N", "_0", "_1", "_4", "_5" }, { "`_N`='" + std::to_string(database_[username]->id_) + "'" });

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

	bool ODBCUserDatabase::addNewUser(const std::string& username, const std::string& password, const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		auto res = odbcDB.SelectValues("local", { "_N" });
		new_id_ = res["_0"] + 1ull;

		if (!addUser(username, password, user_permissions, files_permissions))
			return false;

		odbcDB.InsertValues("user_wright", { "_0", "_1", "_4", "_5" }, { username, md5_from_buffer(password), std::to_string((int)user_permissions), files_permissions.dump() });
		return true;
	}
}