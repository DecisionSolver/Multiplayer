#include <fineftp/server.h>

#include "server_impl.h"
#include <memory>

namespace fineftp
{
	FtpServer::FtpServer(const std::string& address, uint16_t port, const std::string& ftp_working_directory,
		const std::string& DBuser, const std::string& DBpassword,
		const std::string& DBhost, const std::string& DB, const std::string& Table,
		const unsigned short& DBport, const std::string& DBcharset,
		bool DBOnlyRead)
		: ftp_server_(std::make_unique<FtpServerImpl>(address, port, ftp_working_directory, DBuser, DBpassword,
			DBhost, DB, Table, DBport, DBcharset, DBOnlyRead))
	{}

	FtpServer::FtpServer(const std::string& address, uint16_t port, const std::string& ftp_working_directory,
		const std::string& DBdriver, const std::string& DBpath,
		const std::vector<std::string>& DBattributes, const std::string& DBpassword)
		: ftp_server_(std::make_unique<FtpServerImpl>(address, port, ftp_working_directory, DBdriver, DBpath,
			DBattributes, DBpassword))
	{}

	/*FtpServer::FtpServer(uint16_t port)
	  : FtpServer(std::string("0.0.0.0"), port)
	{}*/

	FtpServer::~FtpServer()
	{}

	bool FtpServer::addNewUser(const std::string& username, const std::string& password,
		const UserPermission user_permissions, const nlohmann::json& files_permissions)
	{
		return ftp_server_->addNewUser(username, password, user_permissions, files_permissions);
	}

	bool FtpServer::start(size_t thread_count)
	{
		assert(thread_count > 0);
		return ftp_server_->start(thread_count);
	}

	void FtpServer::stop()
	{
		ftp_server_->stop();
	}

	int FtpServer::getOpenConnectionCount() const
	{
		return ftp_server_->getOpenConnectionCount();
	}

	uint16_t FtpServer::getPort() const
	{
		return ftp_server_->getPort();
	}

	std::string FtpServer::getAddress() const
	{
		return ftp_server_->getAddress();
	}
	std::shared_ptr<UserDatabase> FtpServer::get_ftp_users()
	{
		return ftp_server_->get_ftp_users();
	}
}
