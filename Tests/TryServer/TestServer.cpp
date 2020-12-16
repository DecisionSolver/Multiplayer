#pragma once
#include <iostream>
#include <fstream>
#include "Server.hpp"
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

//New
#include "Packet.hpp"
#include "Server.hpp"
#include "Client.hpp"

using namespace net;
using namespace swl;
#include <conio.h>

#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include <asio/error.hpp>

using asio::ip::udp;
class server
{
public:
	server(asio::io_service& io_service, short port)
		: socket_(io_service, udp::endpoint(asio::ip::address_v4::from_string("192.168.14.1"), (USHORT)20675))
	{
		do_receive();
	}

	void do_receive()
	{
		socket_.async_receive_from(
			asio::buffer(data_, max_length), sender_endpoint_,
			[this](const asio::error_code &ec, std::size_t bytes_recvd)
		{
			if (!ec && bytes_recvd > 0)
			{
				do_send(bytes_recvd);
			}
			else
			{
				do_receive();
			}
		});
	}

	void do_send(std::size_t length)
	{
		socket_.async_send_to(
			asio::buffer(data_, length), sender_endpoint_,
			[this](const asio::error_code &/*ec*/, std::size_t /*bytes_sent*/)
		{
			do_receive();
		});
	}

private:
	udp::socket socket_;
	udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
};

int main(int argc, char* argv[])
{
	try
	{
		asio::io_service io_service;

		server s(io_service, 0);

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
