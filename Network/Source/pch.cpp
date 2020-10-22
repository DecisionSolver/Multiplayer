#include "pch.h"

std::string GetLastErrorStr()
{
	DWORD error = GetLastError();
	if (error)
	{
		LPVOID lp_msg_buf;
		DWORD buf_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lp_msg_buf,
			0, NULL);
		if (buf_len)
		{
			LPCSTR lp_msg_str = (LPCSTR)lp_msg_buf;
			std::string result(lp_msg_str, lp_msg_str + buf_len);

			LocalFree(lp_msg_buf);

			return result;
		}
	}

	return "";
}
