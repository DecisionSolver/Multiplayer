#include "pch.h"
#include <..\Include\Client.h>

#include <algorithm>
#include <cstdlib>

std::mutex m_get_packet, mBlockReleasing, mProccessingChain;

#if defined(USE_SSL)
Client::Client(asio::io_service &IO,
	std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketUDP(IO)
	, m_stopped(false)
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
{
#if __has_include("logger.h")
	Logger_Info_F("Client connection with id {} has been created.\n", m_clientId);
#endif

	m_socketTCP = std::move(socket);

	ftpClient = std::make_shared<FTPClient>();
}
#else
Client::Client(asio::io_service &IO,
	std::shared_ptr<asio::ip::tcp::socket> socket):
	//m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO))
	 m_stopped(false)
	//, m_sendBuffers()
	//, m_activeSendBufferIndex(0)
	, m_sending(false)
{
#if __has_include("logger.h")
	Logger_Info("Client has been created.");
#endif

	//m_socketTCP = std::move(socket);

	ftpClient = std::make_shared<network::ClientFTP>();
}
Client::Client(asio::io_service &IO):
	//m_socketTCP(std::make_shared<asio::ip::tcp::socket>(IO))
	 m_stopped(false)
	//, m_sendBuffers()
	//, m_activeSendBufferIndex(0)
	, m_sending(false)
{
#if __has_include("logger.h")
	Logger_Info("Client has been created.");
#endif

	ftpClient = std::make_shared<network::ClientFTP>();
}
Client::Client(asio::io_service &IO,
	const asio::ip::udp::endpoint &endpoint):
	//m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO, endpoint))
	 m_stopped(false)
	//, m_sendBuffers()
	//, m_activeSendBufferIndex(0)
	, m_sending(false)
{
#if __has_include("logger.h")
	Logger_Info("Client has been created.");
#endif

	ftpClient = std::make_shared<network::ClientFTP>();
}
#endif

Client::~Client()
{
	Connected = false;
#if __has_include("logger.h")
	Logger_Info("Client has been destroyed.\n");
#endif
}

void Client::Start()
{
#if __has_include("logger.h")
	Logger_Info("Awaits Messages.\n");
#endif

	DoReceive();
}

extern std::atomic_bool NoMessageLeft;
extern std::condition_variable cvBlocking;
//extern std::vector<std::shared_ptr<PoolWaiterPackets>> PacketChain;

void Client::Stop(bool NeedLock)
{
#if __has_include("logger.h")
		Logger_Info("Client Stops.\n");
#endif

	std::unique_lock<std::mutex> lock(m_disconnect);
	m_stopped = true;

	if (Connected)
	{
		std::shared_ptr<network::Packet> disconnect = std::make_shared<network::Packet>();
		disconnect->CreatePacket((int)network::Packet::Type::Disconnection, false);
		Send(disconnect);
	}
	SetConnected(false);
	isLogged = false;

	successConn.notify_all();

	if (NeedLock)
	{
		NoMessageLeft.store(false);
		cvBlocking.notify_one();

		Sleep(1000);
	}
}

/*
void Client::Send(const std::vector<char> &data)
{
	// Append to the inactive buffer
	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), data.begin(), data.end());

	DoSend();
}

void Client::Send(std::shared_ptr<network::Packet> packet)
{
	std::vector<char> Data =
		std::vector<char>(packet->getData().dump().data(), packet->getData().dump().data() + packet->getData().dump().size());

	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), Data.begin(), Data.end());

	DoSend();
}
void Client::Send(const void *Data, size_t Size)
{
	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), (char*)Data, (char*)Data + Size);

	DoSend();
}
*/

void Client::GetPacket(network::Packet &packet, const int &_CheckingByType,
	const int &_CheckingByStatus,
	const std::string &_CheckingByData)
{
	std::scoped_lock get_packet(m_get_packet);
	if (!packet_queue.empty())
	{
		NoMessageLeft.store(false);
		auto It = packet_queue.find(_CheckingByType);
		if (!_CheckingByData.empty())
		{
			// If Needs To Find Packet With Another Condition
			It = packet_queue.end();

			for (auto &_It: packet_queue)
			{
				auto obj = _It.second->getData();
				auto res = std::find_if(obj.begin(), obj.end(),
				[_CheckingByData](const nlohmann::json& x)
				{
					return x.contains(_CheckingByData);
				});

				if (res != obj.end())
				{
					packet = *_It.second;
					packet_queue.erase(_It.first);
					return;
				}
			}
		}
		if (_CheckingByStatus != -1)
		{
			// If Needs To Find Packet With Another Condition
			It = packet_queue.end();
			
			for (auto &_It : packet_queue)
			{
				auto obj = _It.second->getData();
				nlohmann::json::const_iterator res = obj.end();
				recursive_iterate(obj, [&](nlohmann::json::const_iterator it)
				{
					if ((*it).type() == nlohmann::json::value_t::number_integer && (*it) == _CheckingByStatus)
					{
						res = it;
						return true;
					}
					return false;
				});
				if (res != obj.end())
				{
					packet = *_It.second;
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
		NoMessageLeft.store(true);
		cvBlocking.notify_one();
	}
}
void Client::GetPacketFromDelayed(network::Packet &packet, const int &_CheckingByType,
	const int &_CheckingByStatus,
	const std::string &_CheckingByData)
{
	std::scoped_lock get_packet(m_get_packet);
	if (!packet_queue_delayed.empty())
	{
		NoMessageLeft.store(false);
		auto It = packet_queue_delayed.find(_CheckingByType);
		if (!_CheckingByData.empty())
		{
			// If Needs To Find Packet With Another Condition
			It = packet_queue_delayed.end();

			for (auto &_It: packet_queue_delayed)
			{
				auto obj = _It.second->getData();
				auto res = std::find_if(obj.begin(), obj.end(),
				[_CheckingByData](const nlohmann::json& x)
				{
					return x.contains(_CheckingByData);
				});

				if (res != obj.end())
				{
					packet = *_It.second;
					packet_queue_delayed.erase(_It.first);
					return;
				}
			}
		}
		if (_CheckingByStatus != -1)
		{
			// If Needs To Find Packet With Another Condition
			It = packet_queue_delayed.end();
			
			for (auto &_It : packet_queue_delayed)
			{
				auto obj = _It.second->getData();
				nlohmann::json::const_iterator res = obj.end();
				recursive_iterate(obj, [&](nlohmann::json::const_iterator it)
				{
					//std::cout << (*it) << std::endl;
					if ((*it).type() == nlohmann::json::value_t::number_integer && (*it) == _CheckingByStatus)
					{
						res = it;
						return true;
					}
					return false;
				});
				if (res != obj.end())
				{
					packet = *_It.second;
					packet_queue_delayed.erase(_It.first);
					return;
				}
			}
		}
		if (It != packet_queue_delayed.end())
		{
			packet = *It->second;
			packet_queue_delayed.erase(It);
		}
	}
	else
	{
		NoMessageLeft.store(true);
		cvBlocking.notify_one();
	}
}

extern std::mutex m_connectionsMutex;

const std::string delim = std::string("}{");

void Client::DisconnectByError(const asio::error_code &errorCode)
{
	std::scoped_lock<std::mutex> lock(m_connectionsMutex);

	//PackerLine->Disconnect();

	IsError.store(true);
	error_queue.push_back(errorCode);
	error.notify_one();

	SetConnected(false);

	NoMessageLeft.store(false);
	cvBlocking.notify_one();
}

/*
void Client::ProccessPacket(const std::shared_ptr<Client> &self)
{
	std::scoped_lock get_packet(m_get_packet);
	try
	{
		std::vector<std::string> packets;
		const std::string &Copy = self->receiveBuffer.data();

		if (Copy.empty())
		{
#if __has_include("logger.h")
			Logger_Debug("One Packet Has Been Empty And Skipped!");
#endif
			return;
		}

		size_t _start = 0, _end = 0;
		// If Delimiter Found
		while ((_start = Copy.find(delim, _end)) != std::string::npos)
		{
			_start++;
			auto found_str = Copy.substr(_end, _start - _end);
			if (!found_str.empty() && found_str.find("header") != std::string::npos)
			{
				packets.push_back(found_str);
			}
			_end = _start;
		}
		// If Nothing Was Find But It's not End Of Data Yet
		if (_end < Copy.length())
		{
			// Try To Add All What's Data Left
			auto another_found_str = Copy.substr(_end);
			if (!another_found_str.empty() && another_found_str.find("header") != std::string::npos)
			{
				packets.push_back(another_found_str);
			}
		}

		if (packets.empty())
		{
			packets.push_back(Copy);
		}

		for (size_t i = 0; i < packets.size(); i++)
		{
			std::shared_ptr<network::Packet> newPacket = std::make_shared<network::Packet>();
			std::stringstream cache;
			cache << packets.at(i);

			if (cache && newPacket->onReceive(cache) && (*newPacket))
			{
				ProccessChain(newPacket);

				std::map<asio::ip::udp::endpoint, std::shared_ptr<Client>>::iterator itConnectionUDP;

				// SERVER

				if ((self->Protocol & (int)TypeProtocol::UDP) && !self->GetLogged())
				{
					itConnectionUDP = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
						ConnectionManager::m_connectionsUDP.end(),
						[&](const std::pair<asio::ip::udp::endpoint, std::shared_ptr<Client>> &ThisConn)
					{
						if (ThisConn.first == self->remote_endpoint())
						{
							return true;
						}

						return false;
					});
					if (newPacket->getHeader().type & (int)network::Packet::Type::Client)
					{
						if (self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
							newPacket->getHeader().type & (int)network::Packet::Type::Client &&
							newPacket->getHeader().IsAnswer)
						{
							// If Not User In Massive
							if (itConnectionUDP == ConnectionManager::m_connectionsUDP.end())
							{
								Client::SharedPtr New = std::make_shared<Client>(self->m_owner,
									self->m_owner->GetIOService(), self->remote_endpoint());
								New->setSocketUDP(self->get_socketUDP());

								New->SetEndPoint(self->remote_endpoint());
								New->packet_queue.insert({ newPacket->getHeader().type, newPacket });
								New->SetConnected(true);
								New->isLogged = false;

								ConnectionManager::m_connectionsUDP[self->remote_endpoint()] = New;
							}
						}
					}

					if (itConnectionUDP != ConnectionManager::m_connectionsUDP.end())
					{
						itConnectionUDP->second->packet_queue.insert({ newPacket->getHeader().type, newPacket });
					}
				}

				self->packet_queue.insert({ newPacket->getHeader().type, newPacket });
			}
		}
	}
	catch (const std::exception &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("void Client::ProccessPacket(...)\nException: {}\n", e.what());
#endif
	}
	//std::this_thread::sleep_for(10ms);

	NoMessageLeft.store(false);
	cvBlocking.notify_one();
	//self->receiveBuffer.clear();
}
*/