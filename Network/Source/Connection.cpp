#include "pch.h"
#include "Connection.h"
#include "ConnMan.h"

#include <algorithm>
#include <cstdlib>

//--------------------------------------------------------------------
size_t Connection::m_nextClientId(0);
std::mutex m_get_packet, mBlockReleasing, mProccessingChain;

//--------------------------------------------------------------------------------------------------
std::ostringstream Connection::ErrorCodeToString(const asio::error_code &errorCode)
{
	std::ostringstream debugMsg;
	debugMsg << "Error Category: " << errorCode.category().name() << ". "
		<< " Error Message: " << errorCode.message() << ". "
		<< " GetLastError: " << GetLastError() << ". "
		<< " WSAGetLastError: " << WSAGetLastError() << ". ";

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
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("Client connection with id {} has been created.\n", m_clientId);
#endif

	m_socketTCP = std::move(socket);

	ftpClient = std::make_shared<FTPClient>();
	//m_receiveBuffer.prepare((size_t)std::numeric_limits<std::size_t>::max);
}
#else
//--------------------------------------------------------------------
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO,
	std::shared_ptr<asio::ip::tcp::socket> socket):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO))
	, m_stopped(false)
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("{} Connection with id {} has been created.\n", connectionManager->GetTypeWork() ==
		ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", m_clientId);
#endif

	m_socketTCP = std::move(socket);

	ftpClient = std::make_shared<network::ClientFTP>();
}
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketTCP(std::make_shared<asio::ip::tcp::socket>(IO))
	, m_stopped(false)
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("{} Connection with id {} has been created.\n", connectionManager->GetTypeWork() ==
		ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", m_clientId);
#endif

	ftpClient = std::make_shared<network::ClientFTP>();
}
Connection::Connection(ConnectionManager *connectionManager, asio::io_service &IO,
	const asio::ip::udp::endpoint &ep):
	m_clientId(m_nextClientId++)
	, m_owner(connectionManager)
	, m_socketUDP(std::make_shared<asio::ip::udp::socket>(IO, ep))
	, m_stopped(false)
	, m_sendBuffers()
	, m_activeSendBufferIndex(0)
	, m_sending(false)
	, m_allReadData()
{
#if __has_include("logger.h")
	Logger_Info_F("{} Connection with id {} has been created.\n", connectionManager->GetTypeWork() ==
		ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", m_clientId);
#endif

	ftpClient = std::make_shared<network::ClientFTP>();
}
#endif

//--------------------------------------------------------------------
Connection::~Connection()
{
	Connected = false;
	// Boost uses RAII, so we don't have anything to do. Let thier destructors take care of business
#if __has_include("logger.h")
	Logger_Info_F("{} Connection with id {} has been destroyed.\n", m_owner->GetTypeWork() ==
		ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", m_clientId);
#endif
}

//--------------------------------------------------------------------
void Connection::Start()
{
#if __has_include("logger.h")
	Logger_Info_F("{} ({}) Awaits Messages.\n", m_owner->GetTypeWork() ==
		ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", m_clientId);
#endif

	if (m_owner && m_owner->GetTypeWork() != ConnectionManager::TypeWorking::Client && m_owner->IsSocketBlocking())
		DoReceive();
	else if (!m_owner->IsSocketBlocking())
		DoReceive();
}

//--------------------------------------------------------------------
extern std::atomic_bool NoMessageLeft;
extern std::condition_variable cvBlocking;
extern std::vector<std::shared_ptr<ConnectionManager::PoolWaiterPackets>> PacketChain;

//extern std::atomic_bool HasConnectionPacket;
//extern std::condition_variable cv_PacketWaiter;
void Connection::Stop(bool NeedLock)
{
	if (m_owner)
	{
#if __has_include("logger.h")
		Logger_Info_F("{} ({}) Stops.\n", m_owner->GetTypeWork() ==
			ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", m_clientId);
#endif
	}

	std::unique_lock<std::mutex> lock(m_disconnect);
	m_stopped = true;

	if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client &&
		Connected)
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
		if (m_owner && m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client)
		{
			NoMessageLeft.store(false);
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
	std::vector<char> Data;
	if (!(m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::VOIP)))
		Data = std::vector<char>(packet->getData().dump().data(), packet->getData().dump().data() +
			packet->getData().dump().size());
	else
	{
		packet->PrepareForVOIP();
		Data = std::vector<char>((char *)packet->getRAWData(), (char *)packet->getRAWData() + packet->getDataSize());
	}

	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), Data.begin(), Data.end());

	DoSend();
}
void Connection::Send(const void *Data, size_t Size)
{
	std::vector<char> &inactiveBuffer = m_sendBuffers[m_activeSendBufferIndex ^ 1];
	inactiveBuffer.insert(inactiveBuffer.end(), (char*)Data, (char*)Data + Size);

	DoSend();
}

void Connection::GetPacket(network::Packet &packet, const int &_CheckingByType,
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
void Connection::GetPacketFromDelayed(network::Packet &packet, const int &_CheckingByType,
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

//--------------------------------------------------------------------
extern std::mutex m_connectionsMutex;

const std::string delim = std::string("}{");

void Connection::DisconnectByError(const asio::error_code &errorCode)
{
	if (m_owner && !m_owner->IsSocketBlocking())
		std::scoped_lock<std::mutex> lock(m_connectionsMutex);

	//std::unique_lock<std::mutex> ul(mBlockReleasing);
	for (size_t i = 0; i < PacketChain.size(); i++)
	{
		PacketChain[i]->NotifyOne();
	}

	IsError.store(true);
	error_queue.push_back(errorCode);
	error.notify_one();

	SetConnected(false);

	NoMessageLeft.store(false);
	cvBlocking.notify_one();
}

void Connection::ProccessChain(std::shared_ptr<network::Packet> packet)
{
	std::unique_lock<std::mutex> ul(mProccessingChain);
	for (size_t i = 0; i < PacketChain.size(); i++)
	{
		// If It's Done Then Skip It
		if (PacketChain[i]->WasActive()) continue;

		// Checking For Status
		if (!PacketChain[i]->GetType().empty() && !PacketChain[i]->GetStatus().empty()
			&& PacketChain[i]->GetType().find(packet->getHeader().type) != PacketChain[i]->GetType().end())
		{
			auto obj = packet->getData();
			nlohmann::json::const_iterator res = obj.end();
			recursive_iterate(obj, [&](nlohmann::json::const_iterator it)
			{
				auto Obj = PacketChain[i]->GetStatus().find((*it));
				if ((*it).type() == nlohmann::json::value_t::number_integer &&
					Obj != PacketChain[i]->GetStatus().end())
				{
					res = it;
					return true;
				}
				return false;
			});
			if (res != obj.end())
			{
				if (PacketChain[i]->GetStatus().find((*res))->second)
				{
					for (size_t n = 0; n < PacketChain.size(); n++)
					{
						PacketChain[n]->SetWasPacket();

						PacketChain[n]->SetStatusCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first,
							PacketChain[i]->GetStatus().find((*res))->first);

						// Unblock "Check" Function
						PacketChain[n]->NotifyOne();
					}
				}
				else
				{
					PacketChain[i]->SetWasPacket();
					
					PacketChain[i]->SetStatusCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first,
						PacketChain[i]->GetStatus().find((*res))->first);

					// Unblock "Check" Function
					PacketChain[i]->NotifyOne();
				}
				break;
			}
		}
		else if (!PacketChain[i]->GetType().empty() &&
			PacketChain[i]->GetType().find(packet->getHeader().type) != PacketChain[i]->GetType().end())
		{
			if (PacketChain[i]->GetType().find(packet->getHeader().type)->second)
			{
				for (size_t n = 0; n < PacketChain.size(); n++)
				{
					PacketChain[n]->SetWasPacket();

					PacketChain[n]->SetTypeCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first);
					
					// Unblock "Check" Function
					PacketChain[n]->NotifyOne();
				}
			}
			else
			{
				PacketChain[i]->SetWasPacket();

				PacketChain[i]->SetTypeCauseBreak(PacketChain[i]->GetType().find(packet->getHeader().type)->first);
				// Unblock "Check" Function
				PacketChain[i]->NotifyOne();
			}
			break;
		}
	}
}

void Connection::DoSend()
{
	//std::unique_lock<std::mutex> lock(muxSend);
	//cvSend.wait(lock, [this] { return !atSend.load(); });
	//atSend.store(true);
	//std::thread([&]
	//{
		// Check if there is an async send in progress
		// An empty active buffer indicates there is no outstanding send
	try
	{
		if (m_sendBuffers[m_activeSendBufferIndex].empty())
		{
			m_activeSendBufferIndex ^= 1;

			std::vector<char> &activeBuffer = m_sendBuffers[m_activeSendBufferIndex];
			if (activeBuffer.empty())
				return;

			auto WriteFunction = [self = shared_from_this()](const asio::error_code &errorCode, size_t bytesTransferred)
			{
				UNREFERENCED_PARAMETER(bytesTransferred);

				if (errorCode)
				{
#if __has_include("logger.h")
					Logger_Error_F("{} An error occured while attemping to send data to {}. Error Code: {}\n",
						self->m_owner->GetTypeWork() ==
						ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]",
						(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
						("client id: " + std::to_string(self->m_clientId)) : "server"),
						ErrorCodeToString(errorCode).str());
#endif
					self->DisconnectByError(errorCode);

					// An error occurred
					// We do not stop or close on sends, but instead let the receive error out and then close
					return;
				}

				if (self->m_stopped || self->IsError.load())
				{
					NoMessageLeft.store(false);
					cvBlocking.notify_one();
					return;
				}

				if (!(self->m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::VOIP)))
				{
					if (self->m_sendBuffers[0].size() > 0)
						self->m_sendBuffers[0].push_back('\0');
					else
						self->m_sendBuffers[1].push_back('\0');
				}

#if __has_include("logger.h")
				Logger_Info_F("{} Sending data to {}: {}\n",
					self->m_owner->GetTypeWork() ==
					ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]",
					(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
						"client id: " + std::to_string(self->m_clientId) : "server"),
					self->m_sendBuffers[0].size() > 0 ?
					self->m_sendBuffers[0].data() :
					self->m_sendBuffers[1].data());
#endif

				self->m_sendBuffers[self->m_activeSendBufferIndex].clear();

				self->start = Time::now();
				// Check if there is more to send that has been queued up on the inactive buffer,
				// while we were sending what was on the active buffer
				if (!self->m_sendBuffers[self->m_activeSendBufferIndex ^ 1].empty() && !self->m_owner->IsSocketBlocking())
					self->DoSend();
			};

			if (m_owner)
			{
				if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::TCP) &&
					!m_owner->IsSocketBlocking())
					m_socketTCP->async_write_some(asio::buffer(activeBuffer.data(), activeBuffer.size()), WriteFunction);
				else if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::UDP) &&
					!m_owner->IsSocketBlocking())
					m_socketUDP->async_send_to(asio::buffer(activeBuffer.data(), activeBuffer.size()), remote_endpoint_,
						WriteFunction);
				else if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::TCP) |
					(int)(ConnectionManager::TypeProtocol::VOIP) && m_owner->IsSocketBlocking())
				{
					size_t st = activeBuffer.size(), len = 0, part = 0;
					asio::error_code ec;
					len = m_socketTCP->write_some(asio::buffer(activeBuffer.data(), st));
					/*
					m_socketTCP->write_some(asio::buffer(&st, sizeof(size_t)), ec);

					while (!activeBuffer.empty())
					{
						st = activeBuffer.size();
						if (st < 4096)
						{
							len += m_socketTCP->write_some(asio::buffer(std::vector<char>{activeBuffer.begin(),
								activeBuffer.end() }.data(), st));
							activeBuffer.erase(activeBuffer.begin(), activeBuffer.begin() + st);
						}
						else
						{
							part = m_socketTCP->write_some(asio::buffer(std::vector<char>{activeBuffer.begin(),
								activeBuffer.begin() + st }.data(), st));
							len += part;
							activeBuffer.erase(activeBuffer.begin(), activeBuffer.begin() + part);
						}
					}
					*/
					auto p = WSAGetLastError();
					WriteFunction(ec, len);
				}
				else if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::UDP) |
					(int)(ConnectionManager::TypeProtocol::VOIP) && m_owner->IsSocketBlocking())
				{
					asio::error_code ec;
					size_t st = m_socketUDP->send_to(asio::buffer(activeBuffer.data(), activeBuffer.size()),
						remote_endpoint_, asio::socket_base::message_end_of_record, ec);
					WriteFunction(ec, st);
				}
			}

			//	delete[] toSendBuff;
			//	toSendBuff = nullptr;
			//}
		}
	}
	catch (const std::exception &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("void Connection::DoSend()\nException: {}\n", e.what());
#endif
	}
	//atSend.store(false);
	//cvSend.notify_one();

	//if (m_owner->IsSocketBlocking())
	//	DoReceive();
//}).detach();
}
void Connection::ProccessPacket(const std::shared_ptr<Connection> &self)
{
	std::scoped_lock get_packet(m_get_packet);
	try
	{
		if (self->m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::VOIP))
		{
			{
				std::shared_ptr<network::Packet> newPacket = std::make_shared<network::Packet>();
				newPacket->onReceive(self->m_receiveBuffer.data(), self->m_receiveBuffer.size());
				newPacket->FromVOIP();

				if (self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client)
					ProccessChain(newPacket);

				std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator itConnectionUDP;
				if (self->m_owner &&
					(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
					((self->m_owner->GetProtocol() & (int)ConnectionManager::TypeProtocol::UDP) && !self->GetLogged())))
				{
					itConnectionUDP = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
						ConnectionManager::m_connectionsUDP.end(),
						[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
					{
						if (ThisConn.first == self->remote_endpoint())
							return true;
						return false;
					});

					if (itConnectionUDP != ConnectionManager::m_connectionsUDP.end())
						itConnectionUDP->second->packet_queue.insert({ newPacket->getHeader().type, newPacket });
				}
				else
					self->packet_queue.insert({ newPacket->getHeader().type, newPacket });
			}
		}
		else
		{
			std::vector<std::string> packets;
			const std::string &Copy = self->m_receiveBuffer.data();

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

				if (cache && newPacket->onReceive(cache) && (*newPacket))
				{
					if (self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Client)
						ProccessChain(newPacket);

					std::map<asio::ip::udp::endpoint, Connection::SharedPtr>::iterator itConnectionUDP;
					if (self->m_owner &&
						(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
						((self->m_owner->GetProtocol() & (int)ConnectionManager::TypeProtocol::UDP) && !self->GetLogged())))
					{
						itConnectionUDP = std::find_if(ConnectionManager::m_connectionsUDP.begin(),
							ConnectionManager::m_connectionsUDP.end(),
							[&](const std::pair<asio::ip::udp::endpoint, Connection::SharedPtr> &ThisConn)
						{
							if (ThisConn.first == self->remote_endpoint())
								return true;
							return false;
						});
						if (newPacket->getHeader().type & (int)network::Packet::Type::Connection)
						{
							if (self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server &&
								newPacket->getHeader().type & (int)network::Packet::Type::Connection &&
								newPacket->getHeader().IsAnswer)
							{
								// If Not User In Massive
								if (itConnectionUDP == ConnectionManager::m_connectionsUDP.end())
								{
									Connection::SharedPtr New = std::make_shared<Connection>(self->m_owner,
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
							itConnectionUDP->second->packet_queue.insert({ newPacket->getHeader().type, newPacket });
					}
					else
						self->packet_queue.insert({ newPacket->getHeader().type, newPacket });
				}
			}
		}
	}
	catch (const std::exception &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("void Connection::ProccessPacket(...)\nException: {}\n", e.what());
#endif
	}
	//std::this_thread::sleep_for(10ms);

	NoMessageLeft.store(false);
	cvBlocking.notify_one();
	self->m_receiveBuffer.clear();
}

std::mutex muxReceiveBuffer;
//--------------------------------------------------------------------
void Connection::DoReceive()
{
	//std::unique_lock<std::mutex> lock(muxReceive);
	//cvReceive.wait(lock, [this] { return !atReceive.load(); });
	//atReceive.store(true);
	//std::thread([&]
	//{
	auto ReadFunction =
		[self = shared_from_this()](const asio::error_code &errorCode, size_t bytesRead)
	{
		if (errorCode)
		{
			// Check if the other side hung up
			if (errorCode == asio::error::make_error_code(asio::error::eof))
			{	// This is not really an error. The client is free to hang up whenever they like
#if __has_include("logger.h")
				Logger_Info_F("{} Client {} has disconnected.\n", self->m_owner->GetTypeWork() ==
					ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", self->m_clientId);
#endif
				NoMessageLeft.store(false);
				cvBlocking.notify_one();
			}
			else
			{
#if __has_include("logger.h")
				Logger_Error_F("{} An error occured while attemping to receive data from {}. Error Code: {}\n",
					self->m_owner->GetTypeWork() ==
					ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]",
					(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
					("client id: " + std::to_string(self->m_clientId)) : "server"),
					ErrorCodeToString(errorCode).str());
#endif

				self->DisconnectByError(errorCode);
			}

			// An error occured
			return;
		}

		if (self->m_stopped || self->IsError.load())
		{
			NoMessageLeft.store(false);
			cvBlocking.notify_one();
			return;
		}

		try
		{
			if (self->m_owner && !self->m_owner->IsSocketBlocking())
			{
				//self->m_receiveBuffer[bytesRead] = '\0';
				//std::istream in(&self->m_receiveData);
				self->m_receiveBuffer.insert(self->m_receiveBuffer.end(), self->m_receiveData.data(),
					self->m_receiveData.data() + bytesRead);
				//self->m_receiveBuffer.resize(bytesRead);

				//self->m_receiveData.commit(bytesRead);
			}

#if __has_include("logger.h")
			Logger_Info_F("{} Received data from {}: {}\n",
				self->m_owner->GetTypeWork() ==
				ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]",
				(self->m_owner->GetTypeWork() == ConnectionManager::TypeWorking::Server ?
					std::string("client id: " + std::to_string(self->m_clientId)) : "server"), self->m_receiveBuffer.data());

			self->end = Time::now();
			fsec fs = self->end - self->start;
			ms d = std::chrono::duration_cast<ms>(fs);

			self->ping = (size_t)d.count();
			Logger_Debug_F("{} It Spent {} Time\n", self->m_owner->GetTypeWork() ==
				ConnectionManager::TypeWorking::Client ? "[CLIENT]" : "[SERVER]", (std::to_string(d.count()) + " ms"));
#endif
			// If Packet Not Empty!
			if (bytesRead > 0)
			{
				if (self->m_owner)
				{
					// Packet Is Full Then Parse It
					if (self->m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::TCP))
					{
						size_t Available =
#if !defined(USE_SSL)
							self->get_socketTCP()->available()
#else
							self->get_socketTCP().lowest_layer().available()
#endif
							;

						if (Available == 0 || self->m_owner->IsSocketBlocking())
							self->ProccessPacket(self);
					}
					if (self->m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::UDP)
						&& self->get_socketUDP()->available() == 0)
						self->ProccessPacket(self);
				}
			}
		}
		catch (const std::exception &e)
		{
#if __has_include("logger.h")
			Logger_Error_F("void Connection::DoReceive()\nException: {}\n", e.what());
#endif
		}
		// And Call Itself
		// Issue the next receive
		if (!self->m_owner->IsSocketBlocking())
			self->DoReceive();
	};

	if (m_owner)
	{
		asio::error_code ec;
		size_t st = 0;
		int p = 0;
		if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::TCP) && !m_owner->IsSocketBlocking())
		{
			m_socketTCP->async_receive(asio::buffer(m_receiveData), ReadFunction);
		}
		else if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::UDP) && !m_owner->IsSocketBlocking())
		{
			//m_socketUDP->async_receive_from(m_receiveBuffer, remote_endpoint_, ReadFunction);
		}
		else if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::TCP) |
			(int)(ConnectionManager::TypeProtocol::VOIP) && m_owner->IsSocketBlocking())
		{
			size_t WholeData = 0;/*, CurrentTransfered = 0, part = 0;
			m_socketTCP->read_some(asio::buffer(&CurrentTransfered, sizeof(size_t)));

			boost::array<char, 4096> data;
			m_receiveData.clear();

			while (m_receiveData.size() < CurrentTransfered)
			{
				part = m_socketTCP->read_some(asio::buffer(data, 4096), ec);
				WholeData += part;
				m_receiveData.insert(m_receiveData.end(), data.begin(), data.begin() + part);
			}
			p = WSAGetLastError();*/
			asio::streambuf response;

			// Read until EOF, writing data to output as we go.
			while (true)
			{
				WholeData += asio::read(*m_socketTCP, response, asio::transfer_at_least(1), ec);
				if (WholeData == 0)
					break;

				//std::istream in(&response);
				//m_receiveData.insert(m_receiveData.end(), std::istreambuf_iterator<char>{in}, {});
				//m_receiveData.resize(WholeData);

				if (!m_socketTCP->available())
					break;
			}
			ReadFunction(ec, WholeData);
		}
		else if (m_owner->GetProtocol() & (int)(ConnectionManager::TypeProtocol::UDP) |
			(int)(ConnectionManager::TypeProtocol::VOIP) && m_owner->IsSocketBlocking())
		{
			//st = m_socketUDP->receive_from(m_receiveBuffer, remote_endpoint_,
			//	asio::socket_base::message_end_of_record, ec);
			//ReadFunction(ec, st);
		}
	}
	//	atReceive.store(false);
	//	cvReceive.notify_one();
	//}).detach();
}

//--------------------------------------------------------------------