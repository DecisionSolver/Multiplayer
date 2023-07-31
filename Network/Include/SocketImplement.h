#pragma once
#include "pch.h"
#include <boost/array.hpp>

#include <asio/ssl.hpp>

#include <asio.hpp>
#include <iostream>

class SocketImplement: public std::enable_shared_from_this<SocketImplement>
{
public:
	virtual bool ConnectAsync(const std::string &IP, const short &Port, asio::io_context &io_context) = 0;
	bool SendAsync();
	bool ReceiveAsync();
	virtual bool WriteSocketAsync(
		const std::function<void(const asio::error_code &, size_t)> &WriteSocketFunction) = 0;
	virtual bool ReadSocketAsync(const std::function<void(const asio::error_code &, size_t)> &ReadSocketFunction) = 0;

	const void *GetRawReadBuffer();
	std::istream GetReadBuffer();

	template <typename T>
	SocketImplement *operator <<(const T &);

	template <typename T>
	std::istream operator >>(const T &data);
protected:
	std::string ErrorCodeString(const asio::error_code &errorCode);

	asio::streambuf read_buffer, write_buffer;

	//std::shared_ptr<asio::ip::udp::socket> Socket_UDP;
private:
	std::function<void(const asio::error_code &, size_t)> WriteSocketFunction;
	std::function<void(const asio::error_code &, size_t)> ReadSocketFunction;
};
