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
	
		virtual mysql::Client &GetClientMysql();
		virtual odbc::ODBC &GetClientODBC();

		virtual bool Connect(const std::string& user, const std::string& password, const std::string& host,
			const std::string& DB, const std::string& Table, const unsigned short& port,
			const std::string& charset, bool OnlyRead);
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
			const std::string& DB, const std::string& Table, const unsigned short& port,
			const std::string& charset, bool OnlyRead) override;
		bool Connect(const std::string& driver, const std::string& path,
			const std::vector<std::string>& attributes, const std::string& password) override
		{
			UNREFERENCED_PARAMETER(driver);
			UNREFERENCED_PARAMETER(path);
			UNREFERENCED_PARAMETER(attributes);
			UNREFERENCED_PARAMETER(password);
			return false;
		}

		bool addNewUser(const std::string& username, const std::string& password,
			const UserPermission user_permissions, const nlohmann::json& files_permissions) override;

		std::string updatePermissions(const std::string& username) override;

		mysql::Client &GetClientMysql() override;

	private:
		mysql::Client mysqlDB;

		bool HaveConnect = false;
	};

	class ODBCUserDatabase : public UserDatabase
	{
	public:
		bool Connect(const std::string& user, const std::string& password, const std::string& host,
			const std::string& DB, const std::string& Table, const unsigned short& port,
			const std::string& charset, bool OnlyRead) override
		{
			UNREFERENCED_PARAMETER(user);
			UNREFERENCED_PARAMETER(password);
			UNREFERENCED_PARAMETER(host);
			UNREFERENCED_PARAMETER(DB);
			UNREFERENCED_PARAMETER(Table);
			UNREFERENCED_PARAMETER(port);
			UNREFERENCED_PARAMETER(charset);
			UNREFERENCED_PARAMETER(OnlyRead);
			return false;
		}

		bool Connect(const std::string& driver, const std::string& path,
			const std::vector<std::string>& attributes, const std::string& password) override;

		bool addNewUser(const std::string& username, const std::string& password,
			const UserPermission user_permissions, const nlohmann::json& files_permissions) override;

		std::string updatePermissions(const std::string& username) override;
		
		odbc::ODBC &GetClientODBC() override;

	private:
		odbc::ODBC odbcDB;

		bool HaveConnect = false;
	};
}