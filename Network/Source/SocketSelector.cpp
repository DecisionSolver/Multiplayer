#include "SocketSelector.hpp"

namespace swl
{
	void SocketSelector::add(Socket& socket)
	{
		SOCKET handle = socket.getHandle();
		if (handle != INVALID_SOCKET)
			sockets.push_back(pollfd{ handle, POLLRDNORM | POLLWRNORM /*| POLLERR | POLLHUP*/ });
	}
	void SocketSelector::remove(Socket& socket)
	{
		SOCKET handle = socket.getHandle();
		if (handle != INVALID_SOCKET)
		{
			for (size_t i = 0; i < sockets.size(); i++)
			{
				if (sockets.at(i).fd == handle)
				{
					sockets.erase(sockets.begin() + i);
					break;
				}
			}
		}
	}
	bool SocketSelector::wait(int32_t millisecond)
	{
		int count = ::WSAPoll(&sockets.front(), sockets.size(), millisecond);
		UINT Err = GetLastError();
		if (Err > 0)
			fprintf(stderr, "Function...\nFile %s\n%s: On Line %s\nSays: failed with error %d: %s\n", __FILE__, __FUNCTION__,
				std::to_string(__LINE__).c_str(), WSAGetLastError(), DecodeError(WSAGetLastError()));
		return count > 0;
	}
	SocketSelector::Status SocketSelector::isReady(Socket& _socket)
	{
		SOCKET handle = _socket.getHandle();
		if (handle != INVALID_SOCKET)
		{
			for (auto& socket : sockets)
			{
				if (socket.fd == handle)
				{
					switch (socket.revents)
					{
					case POLLRDNORM:
						return Status::Read;
					case POLLWRNORM:
						return Status::Write;
					case POLLERR:
						return Status::Error;
					case POLLHUP:
						return Status::Disconnected;
					}
				}
			}
		}
		else
			return Status::Error;
		return Status::NotReady;
	}
	void SocketSelector::clear()
	{
		sockets.clear();
	}
}