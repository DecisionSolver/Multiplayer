/**
 * ASIO echo TCP synchronous client-server
 *
 * Author:   Manny egalli64@gmail.com
 * Info:     http://thisthread.blogspot.com/2018/03/boost-asio-echo-tcp-synchronous-client.html
 * Based on: http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/example/cpp11/echo/blocking_tcp_echo_client.cpp
 *			 http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/example/cpp11/echo/blocking_tcp_echo_server.cpp
 */
#include <cstring>
#include <iostream>
#include <thread>
#include <asio.hpp>
#include <mutex>
#include <boost/array.hpp>
namespace ba = asio;
using ba::ip::tcp;

enum { max_length = 65565 };

#define HAS_VOIP_SERVER
#ifdef HAS_VOIP_SERVER
	#include <SFML/Audio/SoundBuffer.hpp>
	#include <SFML/Audio/SoundBufferRecorder.hpp>
	#include <SFML/Audio/SoundStream.hpp>
	#include <SFML/Audio/Sound.hpp>
	#include <SFML/System/Clock.hpp>
#endif

#include "Client.hpp"
#include "Packet.hpp"
#include "Servers.hpp"

std::shared_ptr<network::Client> This_Client = std::make_shared<network::Client>("", (int)ConnectionManager::TypeProtocol::TCP |
(int)ConnectionManager::TypeProtocol::VOIP, 0);

const int SERVER_MAX_LEN = 2;
const int CLIENT_MAX_LEN = 1024;
const int ECHO_PORT = 50014;
const std::string ECHO_PORT_STR{ std::to_string(ECHO_PORT) };
const std::string HOSTNAME{ "localhost" };

int samplerate = 192000; // It means struct ID
enum SampleRates
{
	sr_44100 = 44100,
	sr_48000 = 48000,
	sr_96000 = 96000,
	sr_192000 = 192000,
};
#ifdef HAS_VOIP_SERVER
bool isPlayback = false;

class Play : public sf::SoundRecorder, public sf::SoundStream
{
public:
	Play(asio::io_service &io, std::string host, int countChannels = 1, int SampleRate = SampleRates::sr_44100) :
		socket(io)
	{
		if (isAvailable())
			setChannelCount(countChannels);
		// Set the sound parameters
		initialize(1, SampleRate);

		socket = asio::ip::tcp::socket{ io };
		tcp::resolver resolver{ io };

		//ba::connect(socket, resolver.resolve(host, ECHO_PORT_STR));

		std::string Login, Pass;
		std::cout << "Enter Your Login: " << std::endl;
		std::cin >> Login;
		std::cout << "Enter Your Password: " << std::endl;
		std::cin >> Pass;

		This_Client->Connect(host, 25565, Login, Pass);
	}
	void work()
	{
		// Start playback
		play();

		// Start receiving audio data
		receiveLoop();
	}

private:
	bool onProcessSamples(const sf::Int16 *samples, std::size_t sampleCount) override
	{
		//if (!This_Client->GetConnect() || !This_Client->GetConnect()->IsConnected()) return false;


		// Pack the audio samples into a network packet
		std::shared_ptr<network::Packet> packet = std::make_shared<network::Packet>();
		packet->CreatePacket((int)network::Packet::Type::VOIP, false);
		packet->onReceive(samples, sampleCount * 2);

		This_Client->GetConnect()->Send(packet->GetBinaryData().getData(), packet->GetBinaryData().getDataSize());
		//asio::write(socket, asio::buffer());
		//This_Client->Send(packet);

		// Send the audio packet to the server
		return true;
	}

	bool onGetData(sf::SoundStream::Chunk& data) override
	{
		//if (!This_Client->GetConnect() || !This_Client->GetConnect()->IsConnected()) return false;
		// No new data has arrived since last update: wait until we get some
		while (m_offset >= m_samples.size())
			std::this_thread::sleep_for(100ms);

		//m_offset = 0;
		//m_tempBuffer.clear();

		// Copy samples into a local buffer to avoid synchronization problems
		// (don't forget that we run in two separate threads)
		{
			std::scoped_lock lock(m_mutex);
			m_tempBuffer.assign(m_samples.begin() + static_cast<std::vector<sf::Int64>::difference_type>(m_offset), m_samples.end());
		}

		// Fill audio data to pass to the stream
		data.samples = m_tempBuffer.data();
		data.sampleCount = m_tempBuffer.size();

		// Update the playing offset
		m_offset += data.sampleCount;
		//m_samples.clear();

		return true;
	}
	void onSeek(sf::Time timeOffset) override
	{
		m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) *
			sf::SoundStream::getSampleRate() * sf::SoundStream::getChannelCount() / 1000;
	}
	void receiveLoop()
	{
		while (true)
		{
			std::scoped_lock lock(m_mutex);
			network::Packet packet = network::Packet();

			//boost::array<char, 65565> data;
			//char *reply = new char[max_length];
			//size_t reply_length = 
			if (This_Client->IsSocketBlocking())
				This_Client->GetConnect()->DoReceive();

			//size_t lenght = asio::read(socket, asio::buffer(data));

			//packet.onReceive(data.data(), lenght);
			//reply[reply_length] = '\0';

			This_Client->GetConnect()->GetPacket(packet, (int)network::Packet::Type::VOIP);

			if (packet.getDataSize() > 0)
			{
				std::size_t sampleCount = packet.getDataSize(true) / 2;

				// Don't forget that the other thread can access the sample array at any time
				// (so we protect any operation on it with the mutex)
				{
					std::size_t oldSize = m_samples.size();
					m_samples.resize(oldSize + sampleCount);
					std::memcpy(&(m_samples[oldSize]), static_cast<const char*>(packet.getRAWData(true)), sampleCount);
				}
			}
			std::this_thread::sleep_for(5ms);
		}
	}

	std::recursive_mutex  m_mutex;
	std::vector<sf::Int16> m_samples;
	std::vector<sf::Int16> m_tempBuffer;
	std::size_t m_offset = 0;
	asio::ip::tcp::socket socket;
};
#endif
std::shared_ptr<network::Server> This_Server = std::make_shared<network::Server>("127.0.0.1",
(int)ConnectionManager::TypeProtocol::TCP | (int)ConnectionManager::TypeProtocol::VOIP);

namespace
{
	void session(tcp::socket socket)
	{
		std::cout << "Opening session" << std::endl;
		try
		{
			for (;;)
			{
				boost::array<char, (SampleRates::sr_192000 * sizeof(sf::Int16)) * 2> data;
				asio::error_code error;
				size_t length = socket.read_some(asio::buffer(data), error);

				if (error == asio::error::eof)
					break; // Connection closed cleanly by peer.
				else if (error)
					throw asio::system_error(error); // Some other error.

				asio::write(socket, asio::buffer(data));
			}
		}
		catch (std::exception& e)
		{
			std::cerr << "Session interrupted: " << e.what() << std::endl;
		}
	}

	void server(ba::io_context& io)
	{
		//tcp::acceptor acceptor{ io, tcp::endpoint(tcp::v4(), ECHO_PORT) };
		
		This_Server->Start();

		std::cout << "Server ready" << std::endl;

		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			//std::thread(session, acceptor.accept()).detach();
		}
	}

	void client(ba::io_context& io, const std::string& host)
	{
		try
		{
			//tcp::socket socket{ io };
			//tcp::resolver resolver{ io };
			//ba::connect(socket, resolver.resolve(host, ECHO_PORT_STR));

			std::shared_ptr<Play> Test = std::make_shared<Play>(io, host);
			if (!Test->start(44100))
			{
				std::cerr << "Failed to start recorder" << std::endl;
				return;
			}
			Test->work();

			while (true)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			/*std::cout << "Enter message: ";
			char *request = new char[max_length];
			std::cin >> request;
			size_t request_length = std::strlen(request);
			asio::write(socket, asio::buffer(request, request_length));
			delete[] request;

			char *reply = new char[max_length];
			size_t reply_length = asio::read(socket, asio::buffer(reply, request_length));
			reply[reply_length] = '\0';

			std::cout << "Reply is: ";
			std::cout << reply;
			std::cout << std::endl;
			delete[] reply;*/
		}
		catch (std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << "\n";
		}
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "rus");
	ba::io_context io;
	std::string type;
	std::cin >> type;
	if (type == "c")
		client(io, HOSTNAME);
	else
		server(io);
}