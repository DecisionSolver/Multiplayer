#include "pch.h"
#include "Connection.h"
#include "ConnMan.h"

#include <algorithm>
#include <cstdlib>

//--------------------------------------------------------------------
size_t Connection::m_nextClientId(0);
std::mutex m_get_packet;

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
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO,
	std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketUDP(IO)
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("Client connection with id %zd has been created.\n", m_clientId);
#endif

	m_socketTCP = std::move(socket);

	ftpClient = std::make_shared<FTPClient>();
	m_receiveBuffer.prepare((size_t)std::numeric_limits<std::size_t>::max);
}
#else

//--------------------------------------------------------------------
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketTCP(std::make_shared<asio::ip::tcp::socket>(IO))
	, m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO))
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("Client connection with id %zd has been created.\n", m_clientId);
#endif
	if (connectionManager->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
		m_socketTCP->set_option(asio::ip::tcp::no_delay(true));

	ftpClient = std::make_shared<FTPClient>();
	//m_receiveBuffer.prepare((size_t)std::numeric_limits<std::size_t>::max);
}
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO, const asio::ip::tcp::endpoint &ep):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketTCP(std::make_shared<asio::ip::tcp::socket>(IO, ep))
	, m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO))
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("Client connection with id %zd has been created.\n", m_clientId);
#endif
	if (connectionManager->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
		m_socketTCP->set_option(asio::ip::tcp::no_delay(true));

	ftpClient = std::make_shared<FTPClient>();
	//m_receiveBuffer.prepare((size_t)std::numeric_limits<std::size_t>::max);
}
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO, const asio::ip::udp::endpoint &ep):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketTCP(std::make_shared<asio::ip::tcp::socket>(IO))
	, m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO, ep))
	, m_stopped(false)
	, m_receiveBuffer()
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
//	Logger_Info_F("Client connection with id %zd has been created.\n", m_clientId);
#endif
	if (connectionManager->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
		m_socketTCP->set_option(asio::ip::tcp::no_delay(true));

	ftpClient = std::make_shared<FTPClient>();
	//m_receiveBuffer.prepare((size_t)std::numeric_limits<std::size_t>::max);
}
#endif

//--------------------------------------------------------------------
Connection::~Connection()
{
	Connected = false;
	// Boost uses RAII, so we don't have anything to do. Let thier destructors take care of business
#if __has_include("logger.h")
	Logger_Info_F("Client connection with id %zd has been destroyed.\n", m_clientId);
#endif
}

//--------------------------------------------------------------------
void Connection::Start()
{
#if __has_include("logger.h")
	Logger_Info_F("Client(%zd) Awaits Messages.\n", m_clientId);
#endif

	DoReceive();
}

//--------------------------------------------------------------------
extern std::atomic_bool NoMessageLeft;
extern std::condition_variable cvBlocking;
void Connection::Stop(bool NeedLock)
{
#if __has_include("logger.h")
	Logger_Info_F("Client(%zd) Stops.\n", m_clientId);
#endif

	std::unique_lock<std::mutex> lock(m_disconnect);
	m_stopped = true;

	if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client &&
		Connected)
	{
		std::shared_ptr<network::Packet> disconnect = std::make_shared<network::Packet>();
		disconnect->CreatePacket(network::Packet::Type::Disconnection, false);
		Send(disconnect);
	}
	SetConnected(false);
	isLogged = false;

	successConn.notify_all();

	if (NeedLock)
	{
		if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client)
		{
			NoMessageLeft = false;
			cvBlocking.notify_one();

			Sleep(1000);
		}
	}
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

void Connection::Send(std::shared_ptr<network::Packet> packet)
{
	packet->getData()["header"]["_R"] = UserID_MetaDB;
	auto Data = packet->getData().dump();

	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), Data.begin(), Data.end());

	DoSend();
}

void Connection::GetPacket(network::Packet &packet, network::Packet::Type _CheckingByType, std::string _CheckingByData)
{
	std::scoped_lock get_packet(m_get_packet);
	if (!packet_queue.empty())
	{
		NoMessageLeft = false;
		auto It = packet_queue.find(_CheckingByType);
		if (!_CheckingByData.empty())
		{
			for (auto &_It: packet_queue)
			{
				if (_It.second && _It.second->getData().dump().find(_CheckingByData) != std::string::npos)
				{
					packet = *It->second;
					packet_queue.erase(_It.first);
					return;
				}
			}
		}
		if (It != packet_queue.end())
		{
			packet = *It->second;
			packet_queue.erase(It);
		}
	}
	else
	{
		NoMessageLeft = true;
		cvBlocking.notify_one();
	}
}

//--------------------------------------------------------------------
extern std::mutex m_connectionsMutex;

static auto const delim = std::string("}{");

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

#if __has_include("logger.h")
				Logger_Error_F("An error occured while attemping to send data to %s. Error Code: %s\n",
					(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
					("client id: " + std::to_string(self->m_clientId)).c_str() : "server"),
					ErrorCodeToString(errorCode).str().c_str());
#endif

				self->getIsError() = true;
				self->error_queue.push_back(errorCode);
				self->get_cv_error().notify_one();

				self->SetConnected(false);

				NoMessageLeft = false;
				cvBlocking.notify_one();

				// An error occurred
				// We do not stop or close on sends, but instead let the receive error out and then close
				return;
			}

			if (self->m_stopped || self->IsError.load())
			{
				NoMessageLeft = false;
				cvBlocking.notify_one();
				return;
			}

			if (self->m_sendBuffers[0].size() > 0)
				self->m_sendBuffers[0].push_back('\0');
			else
				self->m_sendBuffers[1].push_back('\0');

#if __has_include("logger.h")
			Logger_Info_F("Sending data to %s: %s\n",
				(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
					std::string("client id: " + std::to_string(self->m_clientId)).c_str() : "server"),
				self->m_sendBuffers[0].size() > 0 ?
				self->m_sendBuffers[0].data() :
				self->m_sendBuffers[1].data());
#endif

			self->m_sendBuffers[self->m_activeSendBufferIndex].clear();

			self->start = Time::now();
			// Check if there is more to send that has been queued up on the inactive buffer,
			// while we were sending what was on the active buffer
			if (!self->m_sendBuffers[self->m_activeSendBufferIndex ^ 1].empty())
				self->DoSend();
		};

		if (self->m_owner && self->m_owner)
		{
			if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
				m_socketTCP->async_write_some(asio::buffer(activeBuffer), WriteFunction);
			else if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
				m_socketUDP->async_send_to(asio::buffer(activeBuffer), remote_endpoint_, WriteFunction);
		}
	}
}

void Connection::ProccessPacket(const std::shared_ptr<Connection> &self)
{
	std::scoped_lock get_packet(m_get_packet);

	std::vector<std::string> packets;
	const std::string &Copy = self->m_receiveData.str();

	size_t _start = 0, _end = 0;
	// If Delimiter Found
	while ((_start = Copy.find(delim, _end)) != std::string::npos)
	{
		_start++;
		auto found_str = Copy.substr(_end, _start - _end);
		if (!found_str.empty() && found_str.find("header") != std::string::npos)
			packets.push_back(found_str);
		_end = _start;
	}
	// If Nothing Was Find But It's not End Of Data Yet
	if (_end < Copy.length())
	{
		// Try To Add All What's Data Left
		auto another_found_str = Copy.substr(_end);
		if (!another_found_str.empty() && another_found_str.find("header") != std::string::npos)
			packets.push_back(another_found_str);
	}

	if (packets.empty())
		packets.push_back(Copy);

	for (size_t i = 0; i < packets.size(); i++)
	{
		std::shared_ptr<network::Packet> newPacket = std::make_shared<network::Packet>();
		std::stringstream cache;
		cache << packets.at(i);

		if (newPacket->onReceive(cache) && (*newPacket))
		{
			std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator itConnectionUDP;
			if (self->m_owner &&
				(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ||
				(self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP && !self->GetLogged())))
			{
				itConnectionUDP = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
					ConnectionManager::m_connectionsUDP.end(),
					[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
				{
					if (ThisConn.first == self->remote_endpoint())
						return true;
					return false;
				});
				if (newPacket->getHeader().type == network::Packet::Type::Connection)
				{
					if (self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
						newPacket->getHeader().type == network::Packet::Type::Connection &&
						newPacket->getHeader().IsAnswer)
					{
						// If Not User In Massive
						if (itConnectionUDP == ConnectionManager::m_connectionsUDP.end())
						{
							Connection::SharedPtr New = std::make_shared<Connection>(self->m_owner,
								self->m_owner->GetIOService());
							New->setSocketUDP(self->get_socketUDP());
							
							New->SetEndPoint(self->remote_endpoint());
							New->packet_queue.insert(
								std::pair<network::Packet::Type,
								std::shared_ptr<network::Packet>>((network::Packet::Type)newPacket->getHeader().type,
									newPacket));
							New->SetConnected(true);
							New->isLogged = false;

							ConnectionManager::m_connectionsUDP[self->remote_endpoint()] = New;
						}
					}
				}

				if (itConnectionUDP != ConnectionManager::m_connectionsUDP.end())
					itConnectionUDP->second->packet_queue.insert(
						std::pair<network::Packet::Type,
						std::shared_ptr<network::Packet>>((network::Packet::Type)newPacket->getHeader().type,
							newPacket));
			}
			else
				self->packet_queue.insert(
					std::pair<network::Packet::Type, std::shared_ptr<network::Packet>>(
					(network::Packet::Type)newPacket->getHeader().type, newPacket));
		}
	}

	NoMessageLeft = false;
	cvBlocking.notify_one();
	self->m_receiveData.str({});
}

//--------------------------------------------------------------------
void Connection::DoReceive()
{
	auto ReadFunction =
		[&, self = shared_from_this()](const asio::error_code &errorCode, size_t bytesRead)
	{
		if (errorCode)
		{
			std::scoped_lock<std::mutex> lock(m_connectionsMutex);
			// Check if the other side hung up
			if (errorCode == asio::error::make_error_code(asio::error::eof))
			{	// This is not really an error. The client is free to hang up whenever they like
#if __has_include("logger.h")
				Logger_Info_F("Client %zd has disconnected.\n", self->m_clientId);
#endif
				NoMessageLeft = false;
				cvBlocking.notify_one();
			}
			else
			{
#if __has_include("logger.h")
				Logger_Error_F("An error occured while attemping to receive data from %s. Error Code: %s\n",
					(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
					("client id: " + std::to_string(self->m_clientId)).c_str() : "server"),
					ErrorCodeToString(errorCode).str().c_str());
#endif

				self->getIsError() = true;
				self->error_queue.push_back(errorCode);
				self->get_cv_error().notify_one();

				NoMessageLeft = false;
				cvBlocking.notify_one();
			}

			// An error occured
			return;
		}

		if (self->m_stopped || self->IsError.load())
		{
			NoMessageLeft = false;
			cvBlocking.notify_one();
			return;
		}
		
		if (!self->m_receiveBuffer.empty())
			self->m_receiveBuffer.elems[bytesRead] = '\0';
		// Grab the read data
		self->m_receiveData << self->m_receiveBuffer.data();

#if __has_include("logger.h")
		Logger_Info_F("Received data from %s: %s\n",
			(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
				std::string("client id: " + std::to_string(self->m_clientId)).c_str() : "server"),
			std::string(std::istreambuf_iterator<char>(self->m_receiveData), {}).c_str());

		self->end = Time::now();
		fsec fs = self->end - self->start;
		ms d = std::chrono::duration_cast<ms>(fs);

		self->ping = (size_t)d.count();
		Logger_Debug_F("It Spent %s Time\n", (std::to_string(d.count()) + " ms").c_str());
#endif

		size_t size = 0u;
		self->m_receiveData.setf(std::ios::skipws);
		self->m_receiveData.seekg(0, self->m_receiveData.end);
		size = (size_t)self->m_receiveData.tellg();

		self->m_receiveData.seekg(0, self->m_receiveData.beg);

		// If Packet Not Empty!
		if (size > 0 && size != std::string::npos)
		{
			if (self->m_owner)
			{
				// Packet Is Full Then Parse It
				if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
				{
#if !defined(USE_SSL)
					if (self->get_socketTCP()->available() == 0)
						ProccessPacket(self);
#else
					if (self->get_socketTCP().lowest_layer().available() == 0)
						ProccessPacket();
#endif
					ProccessPacket(self);
				}
				if (self->m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP
					&& self->get_socketUDP()->available() == 0)
					ProccessPacket(self);
			}
		}
		// And Call It Itself
		// Issue the next receive
		self->DoReceive();
	};

	if (m_owner)
	{
		if (m_owner->GetProtocol() == ConnectionManager::TypeProtocol::TCP)
			m_socketTCP->async_read_some(asio::buffer(m_receiveBuffer), ReadFunction);
		else if (m_owner->GetProtocol() == ConnectionManager::TypeProtocol::UDP)
			m_socketUDP->async_receive_from(asio::buffer(m_receiveBuffer), remote_endpoint_, ReadFunction);
	}
}

//--------------------------------------------------------------------