#include "Server.hpp"
#include <future>


#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"


#include "MySQL_Client.h"
#include "MySQL_Impl.h"

// for convenience
using json = nlohmann::json;

json Message =
{
	{"header",
		{
			{ "_s",1}, // Settings
			{"_t",0}, // Was 2 // Type Of Packet
			{"_R",0} // ID Recipient
		}
	},
	{"data",
		{
	// The Property Of Following Data
	{"_i","trgffdsfh"}, // Id Of Packet (Needs To Be In MD5)
	{"_o",500}, // Orig Size To Decompress (If Was Decompressed)

	{"body", // All Data Is Here
		{
			{"_0","Hello!"},
			//{"_0","Login: PBAX"},
			//{"_1","Pass: _SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
		}
	}
}
}
},
MySQL_Request =
{
	{"header",
		{
			{ "_s",0}, // Settings
			{"_t",2}, // Was 2 // Type Of Packet
			{"_R",0} // ID Recipient
		}
	},
	{"data",
		{
	// The Property Of Following Data
	{"_i","nvchjfgjhfgf"}, // Id Of Packet (Needs To Be In MD5)
	{"_o",500}, // Orig Size To Decompress (If Was Decompressed)

	{"body", // All Data Is Here
		{
			{"_0","PBAX"},
			{"_1","_SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
		}
	}
}
}
},
Answer_Request =
{
	{"header",
		{
			{ "_s",0}, // Settings
			{"_t",3}, // Was 2 // Type Of Packet
			{"_R",0} // ID Recipient
		}
	},
	{"data",
		{
	// The Property Of Following Data
	{"_i","fewdadzff"}, // Id Of Packet (Needs To Be In MD5)
	{"_o",0}, // Orig Size To Decompress (If Was Decompressed)

	{"body", // All Data Is Here
		{
			{"_0","OK"},
			//{"_0","PBAX"},
			//{"_1","_SUCKMYDICK_"}, // Needs To Be In MD5 (If It's A Password!)
		}
	}
}
}
};

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
	void TCPServer::SendTo(SOCKET sock, const std::shared_ptr<Packet> packet)
	{
		if (packet->getSize() == 0)
			return;
		
		socket.SendTo(sock, packet);
	}

	void TCPServer::run(const IPEndpoint& ip, uint16_t port)
	{
		User->Connect("gb_x_lolola32", "55b2zzada", "mysql101.1gb.ru", "gb_x_lolola32");

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
			uint32_t id = 0;
			uint32_t client_id = 1;
			Socket::Status status;
			while (work)
			{
				if (selector.wait())
				{
					switch (selector.isReady(socket))
					{
					case SocketSelector::Write:
					case SocketSelector::Read:
					{
						//Находим клиента у которого 32 бит не равен 1
						auto client = std::find_if(clients.begin(), clients.end(),
							[&](std::pair<TCPSocket, uint32_t>& client)
						{
							return !(client.second >> 31);
						});
						//Если такого нету то
						if (client == clients.end())
						{
							clients.push_back(std::make_pair(TCPSocket(), client_id++ | 0x80000000));
							socket.accept(clients.back().first);
							selector.add(clients.back().first);
#if defined (_SERVER) && defined (_CONSOLE)
							printf("The New Client Was Connected To This Server\nNow Count: %d", clients.size());
#endif
						}
						else
						{
							(*client).first = TCPSocket();
							(*client).second |= 0x80000000;
							socket.accept((*client).first);
							selector.add((*client).first);
#if defined (_SERVER) && defined (_CONSOLE)
							printf("The New Client Was Connected To This Server\nNow Count: %d", clients.size());
#endif
						}
						// If Needs To Send By ID
						break;
					}
					case SocketSelector::NotReady:
					case SocketSelector::Disconnected:
					{
						for (auto& client : clients)
						{
							uint32_t idSender = client.second & 0x7FFFFFFF;
							if (selector.isReady(client.first) == SocketSelector::Disconnected ||
								selector.isReady(client.first) == SocketSelector::NotReady)
							{
								selector.remove(client.first);
								client.first.close();
								client.second = idSender;
#if defined (_SERVER) && defined (_CONSOLE)
								printf("The Client Was Disconnected From This Server\nNow Count: %d", clients.size());
#endif
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
					std::shared_ptr<Packet> packet = std::make_shared<Packet>();
					for (auto& client : clients)
					{
						uint32_t idSender = client.second & 0x7FFFFFFF;
						if (client.second >> 31/* && selector.isReady(*client.first) == SocketSelector::Read*/)
						{
							status = client.first.receive(packet);
							if (packet->getSize() > 0)
							{
								std::string temp;
								temp = packet->ToString();
								switch (packet->getHeader().type)
								{
								case swl::Packet::Type::Chat:
								{
									json data = json::parse(temp);//["_0"].get<std::string>();
									if (!data.empty())
									{
										// If It's For Chat!!!
										//if (id == 0)
										//{
										for (auto& recipient : clients)
										{
											if (data["data"]["body"]["_0"].is_number_integer())
											{
												if ((recipient.second >> 31) && recipient.second != client.second)
													recipient.first.send(packet);
											}
											else
												recipient.first.send(packet);
										}
										//	continue;
										//}
										////Находим клиента которому нужно отправить по id, и чтоб он был подключен 
										//auto recipient = std::find_if(clients.begin(), clients.end(),
										//	[&](std::pair<TCPSocket, uint32_t>& client)
										//{
										//	return (client.second >> 31) && ((client.second & 0x7FFFFFFF) == id);
										//});
										////Если нашли
										//if (recipient != clients.end())
										//{
										//	//Проверка что отправитель не равен получателю
										//	if ((*recipient).second != client.second)
										//		(*recipient).first.send(packet);
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
										printf(("\nsize: " + std::to_string(Obj.size()) + "\n").c_str());
										//size_t I = 0;
										//for (size_t i = 0; i < Obj.size(); i++)
										//{
										//	printf(("[" + std::to_string(i) + "] = " + Obj.at(i).first + "\n").c_str());

										//	for (auto It : Obj.at(i).second)
										//	{
										//		printf(("\t\t[" + std::to_string(I) + "] = " + It + "\n").c_str());
										//	}
										//}

										// If Successfull Then Send It
										if (!Obj.empty())
										{
											std::shared_ptr<Packet> Answer = std::make_shared<Packet>();
											Answer->FillIn(Answer_Request);
											SendTo(client.first.getHandle(), Answer);
										}
									}
									break;
								}
								default:
									printf(("Unknown type of packet was: " + std::string(__FILE__) + "\n" + (__FUNCTION__) +
										" on line " + std::to_string(__LINE__)).c_str());
									break;
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
	UDPServer::UDPServer() : socket{}
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
				uint32_t id = 0;
				uint32_t client_id = 1;
				std::shared_ptr<Packet> packet = std::make_shared<Packet>();
				IPEndpoint ip;
				uint16_t port = 0;
				Socket::Status status;
				while (work)
				{
					if (selector.wait())
					{
						if (selector.isReady(socket))
						{
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
										clients.push_back({ UDPSocket(), ip, port, client_id++ | 0x80000000 });
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
							if (packet->getSize() > 0)
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