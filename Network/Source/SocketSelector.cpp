#include "SocketSelector.hpp"

namespace swl
{
	SocketSelector::SocketSelector() : sockets{}
	{

	}
	SocketSelector::~SocketSelector()
	{
	}
	void SocketSelector::add(Socket& socket)
	{
		SOCKET handle = socket.getHandle();
		if (handle != INVALID_SOCKET)
			sockets.push_back(pollfd{ handle, POLLRDNORM | POLLWRNORM | POLLERR | POLLHUP });
	}
	void SocketSelector::remove(Socket& socket)
	{
		SOCKET handle = socket.getHandle();
		if (handle != INVALID_SOCKET)
		{
			for (size_t i = 0; i < sockets.size(); i++)
			{
				if (sockets[i].fd == handle)
				{
					sockets.erase(sockets.begin() + i);
					break;
				}
			}
		}
	}
	bool SocketSelector::wait(int32_t millisecond)
	{
		int count = WSAPoll(sockets.data(), sockets.size(), millisecond);
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
					if (socket.revents & POLLRDNORM)
						return Status::Read;
					else if (socket.revents & POLLWRNORM)
						return Status::Write;
					else if (socket.revents & POLLERR)
						return Status::Error;
					else if (socket.revents & POLLHUP)
						return Status::Disconnected;
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