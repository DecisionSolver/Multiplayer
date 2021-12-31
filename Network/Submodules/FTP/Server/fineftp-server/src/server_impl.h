#pragma once

#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include <asio.hpp>

#include <ftp_session.h>

#include <ftp_user.h>
#include <user_database.h>

namespace fineftp
{
  class FtpServerImpl
  {
  public:
    FtpServerImpl(const std::string& address, uint16_t port, const std::string& ftp_working_directory, const std::string& DBuser, const std::string& DBpassword,
        const std::string& DBhost, const std::string& DB, const unsigned short& DBport, const std::string& DBcharset, bool DBOnlyRead);
    FtpServerImpl(const std::string& address, uint16_t port, const std::string& ftp_working_directory, const std::string& DBdriver, const std::string& DBpath,
        const std::vector<std::string>& DBattributes, const std::string& DBpassword);

    ~FtpServerImpl();

    bool addNewUser(const std::string& username, const std::string& password, const UserPermission user_permissions, const nlohmann::json& files_permissions);

    bool start(size_t thread_count = 1);

    void stop();

    int getOpenConnectionCount();

    uint16_t getPort(); 

    std::string getAddress();

  private:
    void acceptFtpSession(std::shared_ptr<FtpSession> ftp_session, asio::error_code const& error);

  private:
    std::shared_ptr<UserDatabase> ftp_users_ = nullptr;

    const uint16_t port_;
    const std::string address_;
    const std::string ftp_working_directory_;

    std::vector<std::thread> thread_pool_;
    asio::io_service         io_service_;
    asio::ip::tcp::acceptor  acceptor_;

    std::atomic<int> open_connection_count_;
  };
}