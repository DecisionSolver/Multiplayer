#pragma once
#include <WinSock2.h>

namespace swl
{
	class Network
	{
	public:
		static bool initialize();
		static void shutdown();
	};
}