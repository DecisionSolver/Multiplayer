#include "pch.h"
#include "Connection.h"
#include "ConnMan.h"

#include <algorithm>
#include <cstdlib>

//--------------------------------------------------------------------
size_t Connection::m_nextClientId(0);
std::mutex m_get_packet;

//--------------------------------------------------------------------
#if defined(USE_SSL)
	Connection::SharedPtr Connection::Create(ConnectionManager *connectionManager,
		std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket)
	{
		return Connection::SharedPtr(new Connection(connectionManager, std::move(socket)));
	}
#else
	Connection::SharedPtr Connection::Create(ConnectionManager *connectionManager,
		asio::ip::tcp::socket &socket)
	{
		return Connection::SharedPtr(new Connection(connectionManager, std::move(socket)));
	}
#endif

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
#if defined(USE_SSL)
	Connection::Connection(ConnectionManager *connectionManager,
		std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket):
		m_clientId(m_nextClientId++)
		, m_owner(connectionManager)
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
	
		m_socketTCP = std::move(socket);

		Curr = Last = std::chrono::high_resolution_clock::now();
		ftpClient = std::make_shared<FTPClient>();
	}
#else
	Connection::Connection(ConnectionManager *connectionManager, asio::ip::tcp::socket socket) :
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
#endif

Connection::Connection(ConnectionManager *connectionManager):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
#if !defined(USE_SSL)
	, m_socketTCP(connectionManager->GetIOService())
#endif
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
		network::Packet disconnect = network::Packet();
		disconnect.FillIn(network::Packet::Header(network::Packet::Type::Disconnection),
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

void Connection::Send(network::Packet &packet)
{
	auto Data = packet.getData().str();

	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), Data.begin(), Data.end());

	DoSend();
}

bool Connection::GetTimer()
{
	if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client) return false;

	Last = std::chrono::high_resolution_clock::now();
	auto Diff = (Last - Curr);

//#if defined(HAS_LOGGER)
//	Logger_Info_F("Client(%zd) Has Time: %lld\n", m_clientId,
//		std::chrono::duration_cast<std::chrono::seconds>(Diff).count());
//#endif

	if (Diff > std::chrono::seconds(60))
		return true;
	
	return false;
}

void Connection::GetPacket(network::Packet &packet, network::Packet::Type _CheckingByType, std::string _CheckingByData)
{
	if (!packet_queue.empty())
	{
		std::lock_guard<std::mutex> get_packet(m_get_packet);
		if (!packet_queue.empty())
		{
			auto It = packet_queue.find(_CheckingByType);
			if (!_CheckingByData.empty())
			{
				for (auto &_It: packet_queue)
				{
					if (_It.second && _It.second.getData().str().find(_CheckingByData) != std::string::npos)
					{
						packet = It->second;
						packet_queue.erase(_It.first);
						return;
					}
				}
			}
			if (It != packet_queue.end())
			{
				packet = It->second;
				packet_queue.erase(It);
			}
		}
	}
}

//--------------------------------------------------------------------
extern std::mutex m_connectionsMutex;
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
				std::scoped_lock<std::mutex> lock(m_connectionsMutex);

#if defined(HAS_LOGGER)
				Logger_Error_F("An error occured while attemping to send data to client id %zd. %s\n",
					self->m_clientId, ErrorCodeToString(errorCode).str().c_str());
#endif

				self->getIsError() = true;
				self->error_queue.push_back(errorCode);
				self->get_cv_error().notify_one();

				self->SetConnected(false);

				if (self->m_owner && (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP &&
					self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server))
				{
					auto itConnectionTCP = std::find_if(ConnectionManager::m_connectionsTCP.begin(),
						ConnectionManager::m_connectionsTCP.end(),
						[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
					{
						if (ThisConn.second == self)
							return true;
						return false;
					});

					if (itConnectionTCP != ConnectionManager::m_connectionsTCP.end())
						ConnectionManager::m_connectionsTCP.erase(itConnectionTCP);
				}

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
#if defined(USE_SSL)
			asio::async_write(*m_socketTCP.get(), asio::buffer(activeBuffer), WriteFunction);
#else
			asio::async_write(m_socketTCP, asio::buffer(activeBuffer), WriteFunction);
#endif
		else if (self->m_owner && self->m_owner &&
			self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
			m_owner->GetSocketUDP().async_send_to(asio::buffer(activeBuffer), remote_endpoint_, WriteFunction);
	}
}

//--------------------------------------------------------------------
void Connection::DoReceive()
{
	auto self(shared_from_this());
	auto ReadFunction =
		[self](const asio::error_code &errorCode, size_t bytesRead)
	{
		UNREFERENCED_PARAMETER(bytesRead);

		if (errorCode)
		{
			std::scoped_lock<std::mutex> lock(m_connectionsMutex);

			if (self->m_owner && (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP &&
				self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server))
			{
				auto itConnectionUDP = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
					ConnectionManager::m_connectionsUDP.end(),
					[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
				{
					if (ThisConn.second == self)
						return true;
					return false;
				});

				if (itConnectionUDP != ConnectionManager::m_connectionsUDP.end())
					ConnectionManager::m_connectionsUDP.erase(itConnectionUDP);
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

			if (self->m_owner && (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP &&
				self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server))
			{
				auto itConnectionTCP = std::find_if(ConnectionManager::m_connectionsTCP.begin(),
					ConnectionManager::m_connectionsTCP.end(),
					[&](const std::pair<asio::ip::tcp::endpoint, Connection::SharedPtr> &ThisConn)
				{
					if (ThisConn.second == self)
						return true;
					return false;
				});

				if (itConnectionTCP != ConnectionManager::m_connectionsTCP.end())
					ConnectionManager::m_connectionsTCP.erase(itConnectionTCP);
			}

			// An error occured
			return;
		}

		// Grab the read data
		std::stringstream data;
		std::istream istream(&self->m_receiveBuffer);
		//if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
		data << istream.rdbuf();

#if defined(HAS_LOGGER)
		Logger_Info_F("Received data from client %zd: %s\n", self->m_clientId, data.str().c_str());
#endif

		network::Packet newPacket = network::Packet();
		if (newPacket.onReceive(data))
		{
			std::lock_guard<std::mutex> get_packet(m_get_packet);
			std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator itConnectionUDP;
			if (self->m_owner &&
				(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ||
				(self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP && !self->GetLogged())))
			{
				if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
				{
					itConnectionUDP = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
						ConnectionManager::m_connectionsUDP.end(),
						[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
					{
						if (ThisConn.first == self->remote_endpoint())
							return true;
						return false;
					});
				}
				if (newPacket.getHeader().type == network::Packet::Type::Connection || network::Packet::Type::MySQL)
				{
					if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
						std::scoped_lock<std::mutex> lock(m_connectionsMutex);
					if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP &&
						self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
						newPacket.getHeader().type == network::Packet::Type::Connection &&
						!newPacket.getHeader().IsAnswer)
					{
						if (itConnectionUDP == ConnectionManager::m_connectionsUDP.end())
						{
							self->SetConnected(true);
							self->isLogged = false;
							Connection::SharedPtr New = Connection::Create(self->m_owner);
							New->SetEndPoint(self->remote_endpoint());
							New->packet_queue.insert(
								std::pair<network::Packet::Type,
								network::Packet>((network::Packet::Type)newPacket.getHeader().type, newPacket));

							ConnectionManager::m_connectionsUDP[self->remote_endpoint()] = New;
						}
					}
					if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP
						&& itConnectionUDP != ConnectionManager::m_connectionsUDP.end())
						itConnectionUDP->second->packet_queue.insert(
							std::pair<network::Packet::Type,
							network::Packet>((network::Packet::Type)newPacket.getHeader().type, newPacket));
					else
						self->packet_queue.insert(
							std::pair<network::Packet::Type,
							network::Packet>((network::Packet::Type)newPacket.getHeader().type, newPacket));
				}
			}
			else
				self->packet_queue.insert(
					std::pair<network::Packet::Type, network::Packet>((network::Packet::Type)newPacket.getHeader().type,
						newPacket));
		}

		// Issue the next receive
		if (!self->m_stopped && !self->IsError.load())
			self->DoReceive();
	};

	if (self->m_owner && self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
#if defined(USE_SSL)
		asio::async_read_until(*m_socketTCP.get(), m_receiveBuffer, "#", ReadFunction);
#else
		asio::async_read_until(m_socketTCP, m_receiveBuffer, "#", ReadFunction);
#endif
	else if (self->m_owner && self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
	{
		asio::streambuf::mutable_buffers_type mutableBuffer =
			m_receiveBuffer.prepare(4096);
		m_owner->GetSocketUDP().async_receive_from(asio::buffer(mutableBuffer), remote_endpoint_, ReadFunction);
	}
}

//--------------------------------------------------------------------