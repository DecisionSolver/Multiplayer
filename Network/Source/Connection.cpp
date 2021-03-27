#include "pch.h"
#include "Connection.h"
#include "ConnMan.h"

#include <algorithm>
#include <cstdlib>

//--------------------------------------------------------------------
size_t Connection::m_nextClientId(0);
std::mutex m_get_packet;

//--------------------------------------------------------------------
Connection::SharedPtr Connection::Create(ConnectionManager *connectionManager, asio::ip::tcp::socket &socket)
{
	return Connection::SharedPtr(new Connection(connectionManager, std::move(socket)));
}
Connection::SharedPtr Connection::Create(ConnectionManager *connectionManager)
{
	return Connection::SharedPtr(new Connection(connectionManager));
}

//--------------------------------------------------------------------------------------------------
std::ostringstream Connection::ErrorCodeToString(const asio::error_code &errorCode)
{
	std::ostringstream debugMsg;
	debugMsg << "Error Category: " << errorCode.category().name() << ". "
		<< " Error Message: " << errorCode.message() << ". ";

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
	, m_socketTCP(std::move(socket))
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if defined(HAS_LOGGER)
	Logger_Info_F("Client connection with id %zd has been created.\n", m_clientId);
#endif
	
	Curr = Last = std::chrono::high_resolution_clock::now();

	ftpClient = std::make_shared<FTPClient>();
}

Connection::Connection(ConnectionManager *connectionManager):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketTCP(connectionManager->GetIOService())
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if defined(HAS_LOGGER)
	Logger_Info_F("Client connection with id %zd has been created.\n", m_clientId);
#endif

	Curr = Last = std::chrono::high_resolution_clock::now();

	ftpClient = std::make_shared<FTPClient>();
}

//--------------------------------------------------------------------
Connection::~Connection()
{
	Connected = false;
	// Boost uses RAII, so we don't have anything to do. Let thier destructors take care of business
#if defined(HAS_LOGGER)
	Logger_Info_F("Client connection with id %zd has been destroyed.\n", m_clientId);
#endif
}

//--------------------------------------------------------------------
void Connection::Start()
{
#if defined(HAS_LOGGER)
	Logger_Info_F("Client(%zd) Awaits Messages.\n", m_clientId);
#endif

	DoReceive();
	Curr = std::chrono::high_resolution_clock::now();
}

//--------------------------------------------------------------------
void Connection::Stop()
{
#if defined(HAS_LOGGER)
	Logger_Info_F("Client(%zd) Stops.\n", m_clientId);
#endif

	std::unique_lock<std::mutex> lock(m_disconnect);
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

#if defined(HAS_LOGGER)
	Logger_Info_F("Client(%zd) Has Time: %lld\n", m_clientId,
		std::chrono::duration_cast<std::chrono::seconds>(Diff).count());
#endif

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

		auto WriteFunction =
			[self](const asio::error_code &errorCode, size_t bytesTransferred)
		{
			UNREFERENCED_PARAMETER(bytesTransferred);

			if (errorCode)
			{
#if defined(HAS_LOGGER)
				Logger_Critical_F("An error occured while attemping to send data to client id %zd. %s\n",
					self->m_clientId, ErrorCodeToString(errorCode).str().c_str());
#endif

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

#if defined(HAS_LOGGER)
			Logger_Info_F("Sending data to client %zd: %s\n",
				self->m_clientId, self->m_sendBuffers[0].size() > 0 ?
				self->m_sendBuffers[0].data() :
				self->m_sendBuffers[1].data());
#endif

			self->m_sendBuffers[self->m_activeSendBufferIndex].clear();

			// Check if there is more to send that has been queued up on the inactive buffer,
			// while we were sending what was on the active buffer
			if (!self->m_sendBuffers[self->m_activeSendBufferIndex ^ 1].empty())
				self->DoSend();
		};

		if (self->m_owner && self->m_owner &&
			self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
			asio::async_write(m_socketTCP, asio::buffer(activeBuffer), WriteFunction);
		else if (self->m_owner && self->m_owner &&
			self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
			m_owner->GetSocketUDP().async_send_to(asio::buffer(activeBuffer), remote_endpoint_, WriteFunction);
	}
}

extern std::mutex m_connectionsMutex;
//--------------------------------------------------------------------
void Connection::DoReceive()
{
	auto self(shared_from_this());
	auto ReadFunction =
		[self](const asio::error_code &errorCode, size_t bytesRead)
	{
		UNREFERENCED_PARAMETER(bytesRead);
		std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator itConnection;

		if (errorCode)
		{
			if (self->m_owner && (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP &&
				self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server))
			{
				itConnection = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
					ConnectionManager::m_connectionsUDP.end(),
					[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
				{
					if (ThisConn.second == self)
						return true;
					return false;
				});

				if (itConnection != ConnectionManager::m_connectionsUDP.end())
					ConnectionManager::m_connectionsUDP.erase(itConnection);
			}
			// Check if the other side hung up
			if (errorCode == asio::error::make_error_code(asio::error::eof))
			{	// This is not really an error. The client is free to hang up whenever they like
#if defined(HAS_LOGGER)
				Logger_Info_F("Client %zd has disconnected.\n", self->m_clientId);
#endif
			}
			else
			{
#if defined(HAS_LOGGER)
				Logger_Error_F("An error occured while attemping to receive data from client id %zd. Error Code: %s\n",
					self->m_clientId, ErrorCodeToString(errorCode).str().c_str());
#endif

				self->getIsError() = true;
				self->error_queue.push_back(errorCode);
				self->get_cv_error().notify_one();
			}

			self->SetConnected(false);

			// An error occured
			return;
		}

		// Grab the read data
		std::string data;
		if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
		{
			std::istream istream(&self->m_receiveBuffer);
			std::getline(istream, data, '#');
		}
		else
			data = std::string((const char *)self->m_receiveBuffer.data().data());
		data += "#";

#if defined(HAS_LOGGER)
		Logger_Info_F("Received data from client %zd: %s\n", self->m_clientId, data.c_str());
#endif

		swl::Packet newPacket = swl::Packet();
		if (!data.empty() && newPacket.onReceive(data))
		{
			std::lock_guard<std::mutex> get_packet(m_get_packet);
			if (self->m_owner &&
				(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ||
				(self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP && !self->GetLogged())))
			{
				if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
				{
					itConnection = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
						ConnectionManager::m_connectionsUDP.end(),
						[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
					{
						if (ThisConn.first == self->remote_endpoint())
							return true;
						return false;
					});
				}
				if (newPacket.getHeader().type == swl::Packet::Type::Connection || swl::Packet::Type::MySQL)
				{
					if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
						std::scoped_lock<std::mutex> lock(m_connectionsMutex);
					if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP &&
						self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
						newPacket.getHeader().type == swl::Packet::Type::Connection &&
						!newPacket.getHeader().IsAnswer)
					{
						if (itConnection == ConnectionManager::m_connectionsUDP.end())
						{
							self->SetConnected(true);
							self->isLogged = false;
							Connection::SharedPtr New = Connection::Create(self->m_owner);
							New->SetEndPoint(self->remote_endpoint());
							New->packet_queue.insert(
								std::pair<swl::Packet::Type,
								swl::Packet>((swl::Packet::Type)newPacket.getHeader().type, newPacket));

							ConnectionManager::m_connectionsUDP[self->remote_endpoint()] = New;
						}
					}
					if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP
						&& itConnection != ConnectionManager::m_connectionsUDP.end())
						itConnection->second->packet_queue.insert(
							std::pair<swl::Packet::Type,
							swl::Packet>((swl::Packet::Type)newPacket.getHeader().type, newPacket));
					else
						self->packet_queue.insert(
							std::pair<swl::Packet::Type,
							swl::Packet>((swl::Packet::Type)newPacket.getHeader().type, newPacket));
				}
			}
			else
				self->packet_queue.insert(
					std::pair<swl::Packet::Type, swl::Packet>((swl::Packet::Type)newPacket.getHeader().type, newPacket));
		}

		// Issue the next receive
		if (!self->m_stopped && !self->IsError.load())
			self->DoReceive();
	};

	if (self->m_owner && self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
		asio::async_read_until(m_socketTCP, m_receiveBuffer, "#", ReadFunction);
	else if (self->m_owner && self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
	{
		asio::streambuf::mutable_buffers_type mutableBuffer =
			m_receiveBuffer.prepare(4096);
		m_owner->GetSocketUDP().async_receive_from(asio::buffer(mutableBuffer), remote_endpoint_, ReadFunction);
	}
}

//--------------------------------------------------------------------