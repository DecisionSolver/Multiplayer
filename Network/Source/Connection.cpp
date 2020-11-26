#include "pch.h"
#include "Connection.h"
#include "ConnMan.h"

#include <algorithm>
#include <cstdlib>

//--------------------------------------------------------------------
size_t Connection::m_nextClientId(0);

//--------------------------------------------------------------------
Connection::SharedPtr Connection::Create(ConnectionManager *connectionManager, asio::ip::tcp::socket &socket)
{
	return Connection::SharedPtr(new Connection(connectionManager, std::move(socket)));
}

//--------------------------------------------------------------------------------------------------
std::ostringstream Connection::ErrorCodeToString(const asio::error_code &errorCode)
{
	std::ostringstream debugMsg;
	debugMsg << "Error Category: " << errorCode.category().name() << ". "
		<< " Error Message: " << errorCode.message() << ". ";

	// IMPORTANT - These comparisons only work if you dynamically link boost libraries
	//             Because boost chose to implement boost::system::error_category::operator == by comparing addresses
	//             The addresses are different in one library and the other when statically linking.
	//
	// We use make_error_code macro to make the correct category as well as error code value.
	// Error code value is not unique and can be duplicated in more than one category.
	if (errorCode == asio::error::make_error_code(asio::error::connection_refused))
		debugMsg << " (Connection Refused)";
	else if (errorCode == asio::error::make_error_code(asio::error::eof))
		debugMsg << " (Remote host has disconnected)";
	else
		debugMsg << " (boost::system::error_code has not been mapped to a meaningful message)";

	return debugMsg;
}

//--------------------------------------------------------------------
Connection::Connection(ConnectionManager *connectionManager, asio::ip::tcp::socket socket):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socket(std::move(socket))
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
	printf("Client connection with id %zd has been created.\n", m_clientId);
	Curr = Last = std::chrono::high_resolution_clock::now();

	ftpClient = std::make_shared<FTPClient>();
}

//--------------------------------------------------------------------
Connection::~Connection()
{
	Connected = false;
	// Boost uses RAII, so we don't have anything to do. Let thier destructors take care of business
	printf("Client connection with id %zd has been destroyed.\n", m_clientId);
}

//--------------------------------------------------------------------
void Connection::Start()
{
	DoReceive();
	Curr = std::chrono::high_resolution_clock::now();
}

//--------------------------------------------------------------------
void Connection::Stop()
{
	std::unique_lock<std::mutex> lock(m_disconnect);
	// The entire connection class is only kept alive, because it is a shared pointer and always has a ref count
	// as a consequence of the outstanding async receive call that gets posted every time we receive.
	// Once we stop posting another receive in the receive handler and once our owner release any references to
	// us, we will get destroyed.
	m_stopped = true;

	if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client &&
		Connected)
	{
		swl::Packet disconnect = swl::Packet();
		disconnect.FillIn(swl::Packet::Header(swl::Packet::Type::Disconnection),
			disconnect.CreateDisconnect()->getData());
		Send(disconnect);
	}
	SetConnected(false);
	isLogged = false;

	successConn.notify_all();

	waiterDisconnection.wait(lock);
}

//--------------------------------------------------------------------
void Connection::Send(const std::vector<char> &data)
{
	// Append to the inactive buffer
	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), data.begin(), data.end());

	//
	DoSend();
}

void Connection::Send(const swl::Packet &packet)
{
	auto Data = packet.getData();
	if (Data.empty()) return;

	// Append to the inactive buffer
	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), Data.begin(), Data.end());

	//
	DoSend();
}

bool Connection::GetTimer()
{
	if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client) return false;

	Last = std::chrono::high_resolution_clock::now();
	auto Diff = (Last - Curr);
	OutputDebugStringA(("\nFrom: " + std::to_string(this->m_clientId) + " Time: " +
		std::to_string(std::chrono::duration_cast<std::chrono::seconds>(Diff).count()) + "\n").c_str());
	if (Diff > std::chrono::seconds(60))
		return true;
	
	return false;
}

void Connection::GetPacket(swl::Packet &packet, swl::Packet::Type _CheckingByType, std::string _CheckingByData)
{
	if (!packet_queue.empty())
	{
		std::lock_guard<std::mutex> get_packet(m_get_packet);
		if (!packet_queue.empty())
		{
			OutputDebugStringA(("\nSize of the packet_map: " + std::to_string(packet_queue.size()) + "\n").c_str());
			auto It = packet_queue.find(_CheckingByType);
			if (!_CheckingByData.empty())
			{
				for (auto _It: packet_queue)
				{
					if (_It.second && _It.second.getData().find(_CheckingByData) != std::string::npos)
					{
						packet = std::move(_It.second);
						packet_queue.erase(_It.first);
						return;
					}
				}
			}
			if (It != packet_queue.end())
			{
				packet = std::move(It->second);
				packet_queue.erase(It);
			}
			OutputDebugStringA(("\nSize of the packet_map: " + std::to_string(packet_queue.size()) + "\n").c_str());
		}
	}
}

//--------------------------------------------------------------------
void Connection::DoSend()
{
	// Check if there is an async send in progress
	// An empty active buffer indicates there is no outstanding send
	if (m_sendBuffers[m_activeSendBufferIndex].empty())
	{
		m_activeSendBufferIndex ^= 1;

		std::vector<char> &activeBuffer = m_sendBuffers[m_activeSendBufferIndex];
		auto self(shared_from_this());
	
		asio::async_write(m_socket, asio::buffer(activeBuffer),
			[self](const asio::error_code &errorCode, size_t bytesTransferred)
		{
			UNREFERENCED_PARAMETER(bytesTransferred);

			if (errorCode)
			{
				printf("An error occured while attemping to send data to client id %zd. %s\n",
					self->m_clientId, ErrorCodeToString(errorCode).str().c_str());
				
				self->getIsError() = true;
				self->error_queue.push_back(errorCode);
				self->get_cv_error().notify_one();
				
				self->SetConnected(false);
				// An error occurred
				// We do not stop or close on sends, but instead let the receive error out and then close
				return;
			}
			
			if (self->m_sendBuffers[0].size() > 0)
				self->m_sendBuffers[0].push_back('\0');
			else
				self->m_sendBuffers[1].push_back('\0');

			printf("Sending data to client %zd: %s\n", self->m_clientId, self->m_sendBuffers[0].size() > 0 ?
				self->m_sendBuffers[0].data() :
				self->m_sendBuffers[1].data());
			self->m_sendBuffers[self->m_activeSendBufferIndex].clear();

			// Check if there is more to send that has been queued up on the inactive buffer,
			// while we were sending what was on the active buffer
			if (!self->m_sendBuffers[self->m_activeSendBufferIndex ^ 1].empty())
				self->DoSend();
		});
	}
}

//--------------------------------------------------------------------
void Connection::DoReceive()
{
	asio::async_read_until(m_socket, m_receiveBuffer, '#',
		[self = shared_from_this()](const asio::error_code &errorCode, size_t bytesRead)
	{
		OutputDebugStringA(("\nSize of the packet_map: " + std::to_string(self->packet_queue.size()) + "\n").c_str());
		UNREFERENCED_PARAMETER(bytesRead);
		if (errorCode)
		{
			// Check if the other side hung up
			if (errorCode == asio::error::make_error_code(asio::error::eof))
				// This is not really an error. The client is free to hang up whenever they like
				printf("Client %zd has disconnected.\n", self->m_clientId);
			else
			{
				printf("An error occured while attemping to receive data from client id %zd. Error Code: %s\n",
					self->m_clientId, ErrorCodeToString(errorCode).str().c_str());
			
				self->getIsError() = true;
				self->error_queue.push_back(errorCode);
				self->get_cv_error().notify_one();
			}

			self->SetConnected(false);

			// An error occured
			return;
		}

		// Grab the read data
		std::istream stream(&self->m_receiveBuffer);
		std::string data;
		std::getline(stream, data, '#');
		data += "#";

		printf("Received data from client %zd: %s\n", self->m_clientId, data.c_str());

		swl::Packet newPacket = swl::Packet();
		if (newPacket.onReceive(data.c_str()))
		{
			if (self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server && !self->GetLogged())
			{
				if (newPacket.getHeader().type == swl::Packet::Type::Connection || swl::Packet::Type::MySQL)
					self->packet_queue.insert(
						std::pair<swl::Packet::Type, swl::Packet>((swl::Packet::Type)newPacket.getHeader().type, newPacket));
			}
			else
				self->packet_queue.insert(
					std::pair<swl::Packet::Type, swl::Packet>((swl::Packet::Type)newPacket.getHeader().type, newPacket));
		}

		// Issue the next receive
		if (!self->m_stopped)
			self->DoReceive();
	});
}

//--------------------------------------------------------------------