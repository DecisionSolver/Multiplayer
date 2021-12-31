#pragma once

#include <mutex>
#include <map>
#include <string>
#include <memory>

#include "ftp_user.h"

#include <mysql/MySQL_Client.h>
#include <ODBC/ODBC.h>

namespace fineftp
{
	class UserDatabase
	{
	public:
		UserDatabase();
		~UserDatabase();

		std::shared_ptr<FtpUser> getUser(const std::string& username, const std::string& password) const;

		virtual bool Connect(const std::string& user, const std::string& password, const std::string& host,
			const std::string& DB, const unsigned short& port, const std::string& charset, bool OnlyRead);
		virtual bool Connect(const std::string& driver, const std::string& path,
			const std::vector<std::string>& attributes, const std::string& password);

		virtual bool addNewUser(const std::string& username, const std::string& password,
			const UserPermission user_permissions, const nlohmann::json& files_permissions) = 0;

		virtual std::string updatePermissions(const std::string& username) = 0;

	protected:

		mutable std::mutex                              database_mutex_;
		std::map<std::string, std::shared_ptr<FtpUser>> database_;
		unsigned long long                              new_id_ = 0;

		bool addUser(const std::string& username, const std::string& password,
			const UserPermission user_permissions, const nlohmann::json& files_permissions);
	};

	class MySQLUserDatabase : public UserDatabase
	{
	public:
		bool Connect(const std::string& user, const std::string& password, const std::string& host, 
			const std::string& DB, const unsigned short& port, const std::string& charset, bool OnlyRead) override;

		bool addNewUser(const std::string& username, const std::string& password,
			const UserPermission user_permissions, const nlohmann::json& files_permissions) override;

	 std::string updatePermissions(const std::string& username) override;

	private:
		mysql::Client mysqlDB;
	};

	class ODBCUserDatabase : public UserDatabase
	{
	public:
		bool Connect(const std::string& driver, const std::string& path,
			const std::vector<std::string>& attributes, const std::string& password) override;

		bool addNewUser(const std::string& username, const std::string& password,
			const UserPermission user_permissions, const nlohmann::json& files_permissions) override;

		std::string updatePermissions(const std::string& username) override;

	private:

		odbc::ODBC odbcDB;
	};
}