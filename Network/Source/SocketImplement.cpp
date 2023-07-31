#include "SocketImplement.h"
#include <memory>

bool SocketImplement::SendAsync()
{
	try
	{
		if (write_buffer.size() > 0)
		{
			if (!WriteSocketFunction)
			{
				WriteSocketFunction = [self = shared_from_this()](const asio::error_code &errorCode, size_t bytesTransferred)
				{
					Logger_Info_F("Transferred Bytes: {}", bytesTransferred);

					if (errorCode)
					{
#if __has_include("logger.h")
						Logger_Error_F("Client Has An error occured while attemping to send data to server. Error Code: {}\n",
							self->ErrorCodeString(errorCode));
#endif
			//self->DisconnectByError(errorCode);

			// An error occurred
			// We do not stop or close on sends, but instead let the receive error out and then close
						return;
					}

#if __has_include("logger.h")
					Logger_Info_F("Sending data to server: {}\n", self->write_buffer.data()
						/*std::string({ buffers_begin(self->write_buffer.data()),
						buffers_end(self->write_buffer.data()) }).c_str()*/);
#endif

				//self->start = Time::now();
					self->WriteSocketAsync(self->WriteSocketFunction);
				};
			}

			return WriteSocketAsync(WriteSocketFunction);
		}
		else
		{
#if __has_include("logger.h")
			Logger_Error("Abort Sending Data Because Buffer Was Empty!\n");
#endif
		}
	}
	catch (const std::exception &exception)
	{
#if __has_include("logger.h")
		Logger_Error_F("Exception: {}\n", exception.what());
#endif
	}

	return false;
}

bool SocketImplement::ReceiveAsync()
{
	try
	{
		if (!ReadSocketFunction)
		{
			ReadSocketFunction = [self = shared_from_this()](const asio::error_code &errorCode, size_t bytesRead)
			{
				Logger_Info_F("Byte Readed: {}", bytesRead);

				if (errorCode)
				{
					// Check if the other side hung up
					if (errorCode == asio::error::make_error_code(asio::error::eof))
					{
						// This is not really an error. The client is free to hang up whenever they like
#if __has_include("logger.h")
						Logger_Info("Client has disconnected.\n");
#endif
				//NoMessageLeft.store(false);
				//cvBlocking.notify_one();
					}
					else
					{
#if __has_include("logger.h")
						Logger_Error_F("Client Has An error occured while attemping to receive data from server. Error Code: {}\n",
							self->ErrorCodeString(errorCode));
#endif

				//DisconnectByError(errorCode);
					}

					// An error occured
					return;
				}

				try
				{
					self->read_buffer.consume(bytesRead);

#if __has_include("logger.h")
					Logger_Info_F("Client Received data from server: {}\n", self->read_buffer.data()
					/* std::string({ buffers_begin(self->read_buffer.data()),
					buffers_end(self->read_buffer.data()) }).c_str() */);

					//end = Time::now();
					//fsec fs = end - start;
					//ms d = std::chrono::duration_cast<ms>(fs);

					//ping = (size_t)d.count();
					//Logger_Debug_F("Packet Spent {} Time\n", (std::to_string(d.count()) + " ms"));
#endif
				// If Packet Not Empty!
					if (bytesRead > 0)
					{
						// Packet Is Full Then Parse It
						/*
						if (Protocol & (int)(TypeProtocol::TCP))
						{
							size_t Available =
		#if !defined(USE_SSL)
								self->Socket_TCP->available()
		#else
								get_socketTCP().lowest_layer().available()
		#endif
								;

							if (Available == 0)
							{
								ProccessPacket(self);
							}
						}*/

					}
				}
				catch (const std::exception &exception)
				{
#if __has_include("logger.h")
					Logger_Error_F("Exception: {}\n", exception.what());
#endif
				}

				// And Call Itself
				// Issue the next receive
				self->ReadSocketAsync(self->ReadSocketFunction);
			};
		}

		return ReadSocketAsync(ReadSocketFunction);
	}
	catch (const std::exception &exception)
	{
#if __has_include("logger.h")
		Logger_Error_F("Exception: {}\n", exception.what());
#endif
	}

	return false;
}

const void *SocketImplement::GetRawReadBuffer()
{
	return read_buffer.data().data();
}

std::istream SocketImplement::GetReadBuffer()
{
	return std::istream(&read_buffer);
}

std::string SocketImplement::ErrorCodeString(const asio::error_code &errorCode)
{
	std::ostringstream debugFormattedMessage;
	debugFormattedMessage << "Error Category: " << errorCode.category().name() << ". "
		<< " Error Message: " << errorCode.message() << ". "
		<< " GetLastError: " << GetLastError() << ". "
		<< " WSAGetLastError: " << WSAGetLastError() << ". ";

	if (errorCode == asio::error::make_error_code(asio::error::connection_refused))
	{
		debugFormattedMessage << " (Client Refused)";
	}
	else if (errorCode == asio::error::make_error_code(asio::error::eof))
	{
		debugFormattedMessage << " (Remote host has disconnected)";
	}
	else
	{
		debugFormattedMessage << " (boost::system::error_code has not been mapped to a meaningful message)";
	}

	return debugFormattedMessage.str();
}

template<typename T>
inline SocketImplement *SocketImplement::operator<<(const T &data)
{
	std::ostream output(&write_buffer);
	output << data;

	return this;
}
template<typename T>
inline std::istream SocketImplement::operator>>(const T &data)
{
	std::istream output(&read_buffer);
	output >> data;

	return data;
}
