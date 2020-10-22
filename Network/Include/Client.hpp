#pragma once
#include "pch.h"
#include "ConnMan.h"
#include "IPEndpoint.hpp"

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))

namespace net
{
	class Client: public ConnectionManager
	{
	public:
		Client(swl::IPEndpoint IP, unsigned Port, size_t numThreads = 2):
		ConnectionManager(ConnectionManager::TypeWorking::Client, IP.toString(), Port, numThreads) {}
		~Client() {}
		
		void Connect(const swl::IPEndpoint& ip, const uint16_t& port, const std::string Login, const std::string Password);
		void Disconnect();
	};
}