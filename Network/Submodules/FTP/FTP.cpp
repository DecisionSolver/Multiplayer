#include "../Include/pch.h"
#include <fineftp/server.h>

#include <sstream>

#include <iostream>
#include <thread>
#include <string>
#include <algorithm>

#include "MySQL/MySQL_Client.h"

#include <direct.h>
#include <boost/filesystem.hpp>

#include "Client.hpp"
using namespace boost::filesystem;

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <Shellapi.h>
#include "ODBC/ODBC.h"

int main()
{
	setlocale(LC_ALL, "Russian");

	for (;;)
	{
		std::string Cmd;
		std::cin >> Cmd;
		if (Cmd == "disable")
		{
			CppLogger::DisablePrintAll();
			Logger_Error("Now It Doesn't Work");
		}
		if (Cmd == "enable")
		{
			CppLogger::EnablePrintAll();
			Logger_Error("Now It Works");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}
