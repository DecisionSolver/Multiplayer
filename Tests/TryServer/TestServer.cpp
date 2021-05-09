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
using namespace network;

#include <string>
#include <iostream>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <asio.hpp>
#include <asio/ssl.hpp>

enum { max_length = 1024 };

class client
{
public:
	client(asio::io_service& io_service,
		asio::ssl::context& context,
		asio::ip::tcp::resolver::iterator endpoint_iterator)
		: socket_(io_service, context)
	{
		socket_.set_verify_mode(asio::ssl::verify_peer);
		socket_.set_verify_callback(
			boost::bind(&client::verify_certificate, this, _1, _2));

		asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
			boost::bind(&client::handle_connect, this,
				asio::placeholders::error));
	}

	bool verify_certificate(bool preverified,
		asio::ssl::verify_context& ctx)
	{
		// The verify callback can be used to check whether the certificate that is
		// being presented is valid for the peer. For example, RFC 2818 describes
		// the steps involved in doing this for HTTPS. Consult the OpenSSL
		// documentation for more details. Note that the callback is called once
		// for each certificate in the certificate chain, starting from the root
		// certificate authority.

		// In this example we will simply print the certificate's subject name.
		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		std::cout << "Verifying " << subject_name << "\n";

		return preverified;
	}

	void handle_connect(const asio::error_code& error)
	{
		if (!error)
		{
			socket_.async_handshake(asio::ssl::stream_base::client,
				boost::bind(&client::handle_handshake, this,
					asio::placeholders::error));
		}
		else
		{
			std::cout << "Connect failed: " << error.message() << "\n";
		}
	}

	void handle_handshake(const asio::error_code& error)
	{
		if (!error)
		{
			std::cout << "Enter message: ";
			std::cin.getline(request_, max_length);
			size_t request_length = strlen(request_);

			asio::async_write(socket_,
				asio::buffer(request_, request_length),
				boost::bind(&client::handle_write, this,
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
		}
		else
		{
			std::cout << "Handshake failed: " << error.message() << "\n";
		}
	}

	void handle_write(const asio::error_code& error,
		size_t bytes_transferred)
	{
		if (!error)
		{
			asio::async_read(socket_,
				asio::buffer(reply_, bytes_transferred),
				boost::bind(&client::handle_read, this,
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
		}
		else
		{
			std::cout << "Write failed: " << error.message() << "\n";
		}
	}

	void handle_read(const asio::error_code& error,
		size_t bytes_transferred)
	{
		if (!error)
		{
			std::cout << "Reply: ";
			std::cout.write(reply_, (std::streamsize)bytes_transferred);
			std::cout << "\n";
		}
		else
		{
			std::cout << "Read failed: " << error.message() << "\n";
		}
	}

private:
	asio::ssl::stream<asio::ip::tcp::socket> socket_;
	char request_[max_length];
	char reply_[max_length];
};
typedef asio::ssl::stream<asio::ip::tcp::socket> ssl_socket;

class session
{
public:
	session(asio::io_service& io_service,
		asio::ssl::context& context)
		: socket_(io_service, context)
	{
	}

	ssl_socket::lowest_layer_type& socket()
	{
		return socket_.lowest_layer();
	}

	void start()
	{
		socket_.async_handshake(asio::ssl::stream_base::server,
			boost::bind(&session::handle_handshake, this,
				asio::placeholders::error));
	}

	void handle_handshake(const asio::error_code& error)
	{
		if (!error)
		{
			socket_.async_read_some(asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
		}
		else
		{
			delete this;
		}
	}

	void handle_read(const asio::error_code& error,
		size_t bytes_transferred)
	{
		if (!error)
		{
			asio::async_write(socket_,
				asio::buffer(data_, bytes_transferred),
				boost::bind(&session::handle_write, this,
					asio::placeholders::error));
		}
		else
		{
			delete this;
		}
	}

	void handle_write(const asio::error_code& error)
	{
		if (!error)
		{
			socket_.async_read_some(asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
					asio::placeholders::error,
					asio::placeholders::bytes_transferred));
		}
		else
		{
			delete this;
		}
	}

private:
	ssl_socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
};

class server
{
public:
	server(asio::io_service& io_service, unsigned short port)
		: io_service_(io_service),
		acceptor_(io_service,
			asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
		context_(asio::ssl::context::sslv23)
	{
		context_.set_options(
			asio::ssl::context::default_workarounds
			| asio::ssl::context::no_sslv2
			| asio::ssl::context::single_dh_use);
		//context_.set_password_callback(boost::bind(&server::get_password, this));
		context_.use_certificate_chain_file("keys/rootca.crt");
		context_.use_private_key_file("keys/rootca.key", asio::ssl::context::pem);
		context_.use_tmp_dh_file("keys/dh2048.pem");

		start_accept();
	}

	std::string get_password() const
	{
		return "test";
	}

	void start_accept()
	{
		session* new_session = new session(io_service_, context_);
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
				asio::placeholders::error));
	}

	void handle_accept(session* new_session,
		const asio::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}
		else
		{
			delete new_session;
		}

		start_accept();
	}

private:
	asio::io_service& io_service_;
	asio::ip::tcp::acceptor acceptor_;
	asio::ssl::context context_;
};

#include <locale.h>
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");
	try
	{
		std::thread(
			[&]
		{
			asio::io_service io_service;
			server s(io_service, 20675);
			io_service.run();
		}).join();

		Sleep(1000);

		asio::io_service io_service;

		asio::ip::tcp::resolver resolver(io_service);
		asio::ip::tcp::resolver::query query("127.0.0.1", "20675");
		asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

		asio::ssl::context ctx(asio::ssl::context::sslv23);
		
		ctx.load_verify_file("keys/rootca.crt");

		client c(io_service, ctx, iterator);

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
