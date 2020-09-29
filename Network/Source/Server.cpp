#include "Server.hpp"
#include <future>


#include "MySQL_Client.h"
#include "MySQL_Impl.h"

namespace swl
{
	std::shared_ptr<TCPServer> Server = std::make_shared<TCPServer>();
	std::shared_ptr<mysql::MYSQLCLIENT> User = std::make_shared<mysql::MYSQLCLIENT>();

	Server::Server() : selector{}
	{
	}
	bool Server::isWork() const
	{
		return work;
	}
	TCPServer::TCPServer() : socket{}
	{

	}
	TCPServer::~TCPServer()
	{

	}
	void TCPServer::SendTo(SOCKET sock, const Packet packet)
	{
		if (packet.getSize() == 0)
			return;
		if (sock == INVALID_SOCKET)
			throw std::exception("INVALID_SOCKET");

		socket.SendTo(sock, packet);
	}
	void TCPServer::SendTo(TCPSocket sock, const Packet packet)
	{
		if (packet.getSize() == 0)
			return;
		if (sock.getHandle() == INVALID_SOCKET)
			throw std::exception("INVALID_SOCKET");

		socket.SendTo(sock.getHandle(), packet);
	}

	void TCPServer::run(const IPEndpoint& ip, uint16_t port)
	{
		User->Connect("gb_z_rod2_rf", "696ea7b8ty", "mysql101.1gb.ru", "gb_z_rod2_rf");

		if (work) return;
		work = true;
		socket = TCPSocket();
		socket.bind(ip, port);
		socket.listen();
		selector.add(socket);
		IP = ip;
		Port = port;
		main = std::thread([&]()
		{
			uint32_t client_id = 1;
			while (work)
			{
				if (selector.wait())
				{
					switch (selector.isReady(socket))
					{
					case SocketSelector::Write:
					case SocketSelector::Read:
					{
						std::thread([&]()
						{
							//Находим клиента у которого 32 бит не равен 1
							auto client = std::find_if(clients.begin(), clients.end(), [&](Client& client)
							{
								return !(client.TCP.second >> 31);
							});
							Packet packet = Packet();
							json data = packet.CreateAnswer();
							data["header"]["_t"] = swl::Packet::Type::Connection;
							data["data"]["body"]["_0"] = "OK";

							uint8_t newType = data["header"]["_t"].get<uint8_t>();
							newType |= (swl::Packet::Type::Answer << swl::Packet::Type::Connection);
							data["header"]["_t"] = newType;

							packet.FillIn(data);
							//Если такого нету то
							if (client == clients.end())
							{
								clients.push_back(Client());
								clients.back().TCP = { TCPSocket(), client_id++ | 0x80000000 };
								socket.accept(clients.back().TCP.first);
								selector.add(clients.back().TCP.first);
#if defined (_SERVER) && defined (_CONSOLE)
								printf("\nThe New Client Was Connected To This Server\nNow Count: %d\n",
									clients.size());
#endif
								clients.back().Handler = std::thread(Client::PacketHandler_TCP,
									this, clients.back().TCP);
								clients.back().Handler.detach();

								std::this_thread::sleep_for(std::chrono::milliseconds(4000));
								SendTo(clients.back().TCP.first, packet);
							}
							else
							{
								(*client).TCP.first = TCPSocket();
								(*client).TCP.second |= 0x80000000;
								socket.accept((*client).TCP.first);
								selector.add((*client).TCP.first);
#if defined (_SERVER) && defined (_CONSOLE)
								printf("\nThe New Client Was Connected To This Server\nNow Count: %d\n",
									clients.size());
#endif
								(*client).Handler = std::thread(Client::PacketHandler_TCP, this, (*client).TCP);
								(*client).Handler.detach();

								std::this_thread::sleep_for(std::chrono::milliseconds(4000));
								clients.back().TCP.first.send(packet);
							}
							//packet.clear();
							//data.clear();
							//data = packet.CreateMessage();
							//data["header"]["_t"] = swl::Packet::Type::Chat;
							//data["data"]["body"]["_0"] = "SERVER: The New Client Has Been Connected!";
							//newType = 0;
							//newType = data["header"]["_t"].get<uint8_t>();
							//newType |= (swl::Packet::Type::Answer << swl::Packet::Type::Chat);
							//data["header"]["_t"] = newType;
							//packet.FillIn(data);
							//for (auto& client : clients)
							//{
							//	SendTo(client.TCP.first, packet);
							//}
						}).detach();
						break;
					}
					case SocketSelector::NotReady:
					case SocketSelector::Disconnected:
					{
						for (auto& client : clients)
						{
							uint32_t idSender = client.TCP.second & 0x7FFFFFFF;
							if (selector.isReady(client.TCP.first) == SocketSelector::Disconnected ||
								selector.isReady(client.TCP.first) == SocketSelector::NotReady)
							{
								selector.remove(client.TCP.first);
								client.TCP.first.close();
								client.TCP.second = idSender;
#if defined (_SERVER) && defined (_CONSOLE)
								printf("\nThe Client Was Disconnected From This Server\nNow Count: %d\n",
									clients.size());
#endif
								//Packet packet = Packet();
								//json data = packet.CreateMessage();
								//data["header"]["_t"] = swl::Packet::Type::Chat;
								//data["data"]["body"]["_0"] = "SERVER: The Client Was Disconnected!";
								//uint8_t newType = data["header"]["_t"].get<uint8_t>();
								//newType |= (swl::Packet::Type::Answer << swl::Packet::Type::Chat);
								//data["header"]["_t"] = newType;
								//packet.FillIn(data);
								//for (auto& client : clients)
								//{
								//	SendTo(client.TCP.first, packet);
								//}
							}
						};
						break;
					}
					case SocketSelector::Error:
						fprintf(stderr, "Function...\nFile %s\n%s: On Line %s\nSays: failed with error %d: %s\n",
							__FILE__, __FUNCTION__, std::to_string(__LINE__).c_str(),
							WSAGetLastError(), DecodeError(WSAGetLastError()));
						break;
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		});
		main.detach();
	}

	SocketSelector TCPServer::Client::selector;
	void TCPServer::Client::PacketHandler_TCP(TCPServer *server, std::pair<TCPSocket, uint32_t> client)
	{
		selector.add(client.first);
		while (server->work)
		{
			if (selector.wait())
			{
				Socket::Status status;
				Packet packet = Packet();
				if (client.second >> 31)
				{
					status = client.first.receive(packet);
					if (status != Socket::Status::Done)
						if (selector.isReady(client.first) == SocketSelector::Error ||
							selector.isReady(client.first) == SocketSelector::Disconnected)
							break;
					if (packet.getSize() > 0)
					{
						std::string temp;
						temp = packet.ToString();
						switch (packet.getHeader().type)
						{
						case swl::Packet::Type::Chat:
						{
							if (!temp.empty())
							{
								json data = json::parse(temp);
								// If It's For Chat!!!
								//if (data["header"]["_R"].is_number_integer())
								//{
								//	////Находим клиента которому нужно отправить по id, и чтоб он был подключен 
								//	auto recipient = std::find_if(server->clients.begin(), server->clients.end(),
								//		[&](Client& client)
								//	{
								//		return (client.TCP.second >> 31) && ((client.TCP.second & 0x7FFFFFFF) ==
								//			data["header"]["_R"].get<size_t>());
								//	});
								//	//Если нашли
								//	if (recipient != server->clients.end())
								//	{
								//		//Проверка что отправитель не равен получателю
								//		if ((*recipient).TCP.second != client.second)
								//			(*recipient).TCP.first.send(packet);
								//	}
								//}
								//else
								//{
									for (auto& recipient : server->clients)
									{
										recipient.TCP.first.send(packet);
									}
								//}
							}
							break;
						}
						case swl::Packet::Type::MySQL:
						{
							if (!temp.empty())
							{
								std::string Login = json::parse(temp)["data"]["body"]["_0"].get<std::string>(),
									Pass = json::parse(temp)["data"]["body"]["_1"].get<std::string>();

								auto Obj = User->TrySelectValues("Local", { "_0", "_1" }, " WHERE _0 = '" + Login
									+ "' AND _1 = '" + Pass + "'");
								//printf(("\nsize: " + std::to_string(Obj.size()) + "\n").c_str());

								// If Successfull Then Send It
								Packet Answer = Packet();
								json pack = Answer.CreateAnswer();
								if (!Obj.empty() && !Obj.front().second.empty())
									pack["data"]["body"]["_0"] = "OK";
								else
									pack["data"]["body"]["_0"] = "NotFound";
								uint8_t newType = pack["header"]["_t"].get<uint8_t>();
								newType |= (swl::Packet::Type::Answer << swl::Packet::Type::MySQL);
								pack["header"]["_t"] = newType;
								Answer.FillIn(pack);
								server->SendTo(client.first, Answer);
								
								if (pack["data"]["body"]["_0"] == "NotFound")
									client.first.close();
							}
							break;
						}
						case swl::Packet::Type::File:
						{
							if (!temp.empty())
							{
								json data = json::parse(temp);
								if (data["data"]["_i"] <= 255)
								{
									// If It's For All
									if (data["header"]["_R"].is_number_integer())
									{	
										////Находим клиента которому нужно отправить по id, и чтоб он был подключен 
										auto recipient = std::find_if(server->clients.begin(), server->clients.end(),
											[&](Client& client)
										{
											return (client.TCP.second >> 31) && ((client.TCP.second & 0x7FFFFFFF) ==
												data["header"]["_R"].get<size_t>());
										});
										//Если нашли
										if (recipient != server->clients.end())
										{
											//Проверка что отправитель не равен получателю
											if ((*recipient).TCP.second == client.second)
											{
												std::this_thread::sleep_for(std::chrono::milliseconds(2000));
												
												swl::Packet AnswerPacket = swl::Packet();
												json dataJSON = AnswerPacket.CreateMessage();
												dataJSON["data"]["body"]["_1"] = "OK";
												uint8_t newType = dataJSON["header"]["_t"].get<uint8_t>();
												newType |= (swl::Packet::Type::Answer << swl::Packet::Type::File);
												dataJSON["header"]["_t"] = newType;
												AnswerPacket.FillIn(dataJSON);

												(*recipient).TCP.first.send(packet);
												continue;
											}

											std::this_thread::sleep_for(std::chrono::milliseconds(2000));
											json dataJSON = json::parse(packet.ToString());
											dataJSON["data"]["body"]["_1"] = "OK";
											uint8_t newType = dataJSON["header"]["_t"].get<uint8_t>();
											newType |= (swl::Packet::Type::Answer << swl::Packet::Type::File);
											dataJSON["header"]["_t"] = newType;
											packet.FillIn(dataJSON);

											(*recipient).TCP.first.send(packet);
										}
									}
									else
									{
										for (auto& recipient : server->clients)
										{
											if (recipient.TCP.second == client.second)
											{
												std::this_thread::sleep_for(std::chrono::milliseconds(2000));

												swl::Packet AnswerPacket = swl::Packet();
												json dataJSON = AnswerPacket.CreateMessage();
												dataJSON["data"]["body"]["_1"] = "OK";
												uint8_t newType = dataJSON["header"]["_t"].get<uint8_t>();
												newType |= (swl::Packet::Type::Answer << swl::Packet::Type::File);
												dataJSON["header"]["_t"] = newType;
												AnswerPacket.FillIn(dataJSON);

												recipient.TCP.first.send(AnswerPacket);
												continue;
											}

											std::this_thread::sleep_for(std::chrono::milliseconds(2000));
											json dataJSON = json::parse(packet.ToString());
											dataJSON["data"]["body"]["_1"] = "OK";
											uint8_t newType = dataJSON["header"]["_t"].get<uint8_t>();
											newType |= (swl::Packet::Type::Answer << swl::Packet::Type::File);
											dataJSON["header"]["_t"] = newType;

											packet.clear();
											packet.FillIn(dataJSON);

											recipient.TCP.first.send(packet);
										}
									}
								}
							}
						}
						break;
						default:
							printf(("Unknown type of packet was: " + std::string(__FILE__) + "\n" + (__FUNCTION__) +
								" on line " + std::to_string(__LINE__)).c_str());
							break;
						}
					}
				}
			}
			else if (selector.isReady(client.first) == SocketSelector::Error ||
				selector.isReady(client.first) == SocketSelector::Disconnected)
				break;
			Sleep(10);
		}
		selector.remove(client.first);
	}
	void TCPServer::stop()
	{
		work = false;
		if (main.joinable())
			main.join();
		socket.close();
	}
	UDPServer::UDPServer() : socket{}
	{

	}
	UDPServer::~UDPServer()
	{
	}
	//void UDPServer::run(const IPEndpoint& ip, uint16_t port)
	//{
	//	if (work) return;
	//	work = true;
	//	socket.bind(ip, port);
	//	selector.add(socket);
	//	main = std::thread([&]()
	//		{
	//			uint32_t id = 0;
	//			uint32_t client_id = 1;
	//			std::shared_ptr<Packet> packet = std::make_shared<Packet>();
	//			IPEndpoint ip;
	//			uint16_t port = 0;
	//			Socket::Status status;
	//			while (work)
	//			{
	//				if (selector.wait())
	//				{
	//					if (selector.isReady(socket))
	//					{
	//						status = socket.receive(packet, ip, port);
	//						port = ntohs(port);
	//						//Находим отправителя пакета в clients
	//						auto sender = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
	//							{
	//								return std::get<1>(client) == ip && std::get<2>(client) == port && (std::get<3>(client) >> 31);
	//							});
	//						//Если его там нету
	//						if (sender == clients.end())
	//						{
	//							//Хотим соединится
	//							if (id == 0x7FFFFFFF)
	//							{
	//								//Находим клиента у которого 32 бит не равен 1
	//								auto client = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
	//								{
	//									return !(std::get<3>(client) >> 31);
	//								});
	//								//Если такого нету
	//								if (client == clients.end())
	//								{
	//									clients.push_back(Client());
	//									clients.back().UPD = { UDPSocket(), ip, port, client_id++ | 0x80000000 };
	//								}
	//								else
	//								{
	//									ToDo("Think It Over!");
	//									//std::get<0>(*client) = UDPSocket();
	//									//std::get<1>(*client) = ip;
	//									//std::get<2>(*client) = port;
	//									//std::get<3>(*client) |= 0x80000000;
	//								}
	//							}
	//							continue;
	//						}
	//						else
	//						{
	//							//Соединение разорвано
	//							if (id == 0x7FFFFFFF || status != Socket::Done)
	//							{
	//								ToDo("Think It Over!");
	//								//std::get<0>(*sender).close();
	//								//std::get<3>(*sender) &= 0x7FFFFFFF;
	//								continue;
	//							}
	//						}
	//						/*
	//						if (packet->getSize() > 0)
	//						{
	//							sender = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
	//							{
	//								return std::get<1>(client) == ip && std::get<2>(client) == port;
	//							});
	//							uint32_t idSender = std::get<3>(*sender) & 0x7FFFFFFF;
	//							if (id == 0)
	//							{
	//								for (auto& client : clients)
	//									if (std::get<1>(client) != ip || std::get<2>(client) != port) //Проверяем что client != sender
	//									{
	//										std::get<0>(client).send(packet, std::get<1>(client), std::get<2>(client));
	//									}
	//								continue;
	//							}
	//							//Находим того, кому надо отправить по id
	//							auto recipient = std::find_if(clients.begin(), clients.end(), [&](std::tuple<UDPSocket, IPEndpoint, uint16_t, uint32_t>& client)
	//								{
	//									return (std::get<3>(client) >> 31) && (std::get<3>(client) & 0x7FFFFFFF) == id;
	//								});
	//							//Если есть получатель
	//							if (recipient != clients.end())
	//							{
	//								if (std::get<1>(*recipient) != ip || std::get<2>(*recipient) != port) //Проверяем что recepient != sender
	//								{
	//									std::get<0>(*recipient).send(packet, std::get<1>(*recipient), std::get<2>(*recipient));
	//								}
	//							}
	//						}
	//						*/
	//					}
	//				}
	//			}
	//		});
	//	main.detach();
	//}
	void UDPServer::stop()
	{
		work = false;
		if (main.joinable())
			main.join();
		socket.close();
	}
}