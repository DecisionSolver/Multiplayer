#pragma once
#include <SocketImplement.h>

class SocketTCP: public SocketImplement
{
public:
	bool ConnectAsync(const std::string &IP, const short &Port, asio::io_context &io_context) override;
	bool WriteSocketAsync(const std::function<void(const asio::error_code &, size_t)> &WriteSocketFunction) override;
	bool ReadSocketAsync(const std::function<void(const asio::error_code &, size_t)> &ReadSocketFunction) override;

	std::shared_ptr<asio::ip::tcp::socket> GetRawSocket()
	{
		return Socket_TCP;
	}
private:
	std::shared_ptr<asio::ip::tcp::socket> Socket_TCP;
};
