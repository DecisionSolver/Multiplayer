#include "Server.hpp"
#include <future>

namespace swl
{
	Server::Server() : selector{}, work{}
	{
	}
	Server::~Server()
	{

	}
	bool Server::isWork() const
	{
		return work;
	}
	TCPServer::TCPServer() : socket(), clients{}
	{

	}
	TCPServer::~TCPServer()
	{

	}
	void TCPServer::run(const IPEndpoint& ip, uint16_t port)
	{
		if (work) return;
		work = true;
		selector.add(socket);
		socket.bind(ip, port);
		socket.listen();
		main = std::thread([&]()
			{
				uint32_t id{};
				Packet packet;
				uint32_t client_id = 1;
				Socket::Status status;
				while (work)
				{
					if (selector.wait())
					{
						if (selector.isReady(socket) == SocketSelector::Read)
						{
							//Находим клиента у которого 32 бит не равен 1
							auto client = std::find_if(clients.begin(), clients.end(), [&](std::pair<TCPSocket, uint32_t>& client)
								{
									return !(client.second >> 31);
								});
							//Если такого нету то
							if (client == clients.end())
							{
								clients.push_back(std::make_pair(TCPSocket(), client_id++ | 0x80000000));
								socket.accept(clients.back().first);
								selector.add(clients.back().first);
							}
							else
							{
								(*client).first = TCPSocket();
								(*client).second |= 0x80000000;
								socket.accept((*client).first);
								selector.add((*client).first);
							}
						}
						else
						{
							for (auto& client : clients)
							{
								uint32_t idSender = client.second & 0x7FFFFFFF;
								if (selector.isReady(client.first) == SocketSelector::Disconnected)
								{
									selector.remove(client.first);
									client.first.close();
									client.second = idSender;
									continue;
								}
							}
							for (auto& client : clients)
							{
								uint32_t idSender = client.second & 0x7FFFFFFF;
								if (client.second >> 31 && selector.isReady(client.first) == SocketSelector::Read)
								{
									status = client.first.receiveAll((void*)&id, 4);
									status = client.first.receive(packet);
									if (packet.getSize() > 0)
									{
										if (id == 0)
										{
											for (auto& recipient : clients)
											{
												if ((recipient.second >> 31) && recipient.second != client.second)
												{
													recipient.first.sendAll((void*)&idSender, 4);
													recipient.first.send(packet);
												}
											}
											continue;
										}
										//Находим клиента которому нужно отправить по id, и чтоб он был подключен 
										auto recipient = std::find_if(clients.begin(), clients.end(), [&](std::pair<TCPSocket, uint32_t>& client)
											{
												return (client.second >> 31) && ((client.second & 0x7FFFFFFF) == id);
											});
										//Если нашли
										if (recipient != clients.end())
										{
											//Проверка что отправитель не равен получателю
											if ((*recipient).second != client.second)
											{
												(*recipient).first.sendAll((void*)&idSender, 4);
												(*recipient).first.send(packet);
											}
										}

									}
								}
							}
						}
					}
				}
			});
		main.detach();
	}
	void TCPServer::core()
	{

	}
	void TCPServer::stop()
	{
		work = false;
		if (main.joinable())
			main.join();
		socket.close();
	}
	UDPServer::UDPServer() : clients{}, socket{}
	{

	}
	UDPServer::~UDPServer()
	{
	}
	void UDPServer::run(const IPEndpoint& ip, uint16_t port)
	{
		if (work) return;
		work = true;
		socket.bind(ip, port);
		selector.add(socket);
		main = std::thread([&]()
			{
				uint32_t id{};
				uint32_t client_id = 1;
				Packet packet;
				IPEndpoint ip;
				uint16_t port;
				Socket::Status status;
				while (work)
				{
					if (selector.wait())
					{
						if (selector.isReady(socket))
						{
							status = socket.receiveAll((void*)&id, 4, ip, port);
							status = socket.receive(packet, ip, port);
							port = ntohs(port);
							//Находим отправителя пакета в clients
							auto sender = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
								{
									return std::get<1>(client) == ip && std::get<2>(client) == port && (std::get<3>(client) >> 31);
								});
							//Если его там нету
							if (sender == clients.end())
							{
								//Хотим соединится
								if (id == 0x7FFFFFFF)
								{
									//Находим клиента у которого 32 бит не равен 1
									auto client = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
										{
											return !(std::get<3>(client) >> 31);
										});
									//Если такого нету
									if (client == clients.end())
									{
										clients.push_back(std::make_tuple(UDPSocket(), ip, port, client_id++ | 0x80000000));
									}
									else
									{
										std::get<0>(*client) = UDPSocket();
										std::get<1>(*client) = ip;
										std::get<2>(*client) = port;
										std::get<3>(*client) |= 0x80000000;
									}
								}
								continue;
							}
							else
							{
								//Соединение разорвано
								if (id == 0x7FFFFFFF || status != Socket::Done)
								{
									std::get<0>(*sender).close();
									std::get<3>(*sender) &= 0x7FFFFFFF;
									continue;
								}
							}
							if (packet.getSize() > 0)
							{
								sender = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
									{
										return std::get<1>(client) == ip && std::get<2>(client) == port;
									});
								uint32_t idSender = std::get<3>(*sender) & 0x7FFFFFFF;
								if (id == 0)
								{
									for (auto& client : clients)
										if (std::get<1>(client) != ip || std::get<2>(client) != port) //Проверяем что client != sender
										{
											std::get<0>(client).sendAll((const void*)&idSender, 4, std::get<1>(client), std::get<2>(client));
											std::get<0>(client).send(packet, std::get<1>(client), std::get<2>(client));
										}
									continue;
								}
								//Находим того, кому надо отправить по id
								auto recipient = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
									{
										return (std::get<3>(client) >> 31) && (std::get<3>(client) & 0x7FFFFFFF) == id;
									});

								//Если есть получатель
								if (recipient != clients.end())
								{
									if (std::get<1>(*recipient) != ip || std::get<2>(*recipient) != port) //Проверяем что recepient != sender
									{
										std::get<0>(*recipient).sendAll((const void*)&idSender, 4, std::get<1>(*recipient), std::get<2>(*recipient));
										std::get<0>(*recipient).send(packet, std::get<1>(*recipient), std::get<2>(*recipient));
									}
								}
							}
						}
					}
				}
			});
		main.detach();
	}
	void UDPServer::stop()
	{
		work = false;
		if (main.joinable())
			main.join();
		socket.close();
	}
}