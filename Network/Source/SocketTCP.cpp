#include "SocketTCP.h"

bool SocketTCP::ConnectAsync(const std::string &IP, const short &Port, asio::io_context &io_context)
{
	try
	{
		if (!IP.empty())
		{
			if (!Socket_TCP)
			{
				Socket_TCP = std::make_shared<asio::ip::tcp::socket>(io_context);
			}

			asio::ip::tcp::resolver(io_context).async_resolve(
				asio::ip::tcp::resolver::query(IP, std::to_string(Port)),
				[&](const asio::error_code &errorCode, asio::ip::tcp::resolver::iterator Iterator)
			{
				Logger_Error_F("Error With Resolving IP's: {} ({}), Error Messge:", Iterator->host_name(),
					Iterator->endpoint(), ErrorCodeString(errorCode));

				if (!errorCode)
				{
					asio::async_connect(*Socket_TCP, Iterator,
						[&](const asio::error_code &errorCode, asio::ip::tcp::resolver::iterator Iterator)
					{
						if (errorCode)
						{
							Logger_Error_F("Error With Connect To: {} ({}), Error Messge:", Iterator->host_name(),
								Iterator->endpoint(), ErrorCodeString(errorCode));
						}
						else
						{
							Logger_Info_F("Connected to: {}", Iterator->endpoint());
						}
					});
				}
			});
			return true;
		}
	}
	catch (const std::exception &exception)
	{
#if __has_include("logger.h")
		Logger_Error_F("Exception: {}\n", exception.what());
#endif
	}

	return false;
}

bool SocketTCP::WriteSocketAsync(const std::function<void(const asio::error_code &, size_t)> &WriteSocketFunction)
{
	try
	{
		if (Socket_TCP && WriteSocketFunction)
		{
			Socket_TCP->async_write_some(write_buffer, WriteSocketFunction);
			return true;
		}
	}
	catch (const std::exception &exception)
	{
#if __has_include("logger.h")
		Logger_Error_F("Exception: {}\n", exception.what());
#endif
	}

	return false;
}

bool SocketTCP::ReadSocketAsync(const std::function<void(const asio::error_code &, size_t)> &ReadSocketFunction)
{
	try
	{
		if (Socket_TCP && ReadSocketFunction)
		{
			Socket_TCP->async_receive(read_buffer, ReadSocketFunction);
			return true;
		}
	}
	catch (const std::exception &exception)
	{
#if __has_include("logger.h")
		Logger_Error_F("Exception: {}\n", exception.what());
#endif
	}

	return false;
}
