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

	path local_root = _getcwd(nullptr, UINT16_MAX); // The backslash at the end is necessary!

	local_root += "/";
	if (!exists(local_root.string() + "Workspace"))
		create_directories(local_root.string() + "Workspace/");
	local_root += "Workspace/";

	if (!exists(local_root.string() + "updates"))
		create_directories(local_root.string() + "updates/");

	local_root = local_root.generic_path();

#if defined(HAS_LOGGER)
	if (!exists(local_root.string() + "logs"))
		create_directories(local_root.string() + "logs/");

	CppLogger::registerTarget(
		new FileLoggerTarget(local_root.string() + "logs/FTP-server-all.log", LogLevel::LOG_LEVEL_DEBUG));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root.string() + "logs/FTP-server-err.log", LogLevel::LOG_LEVEL_ERROR));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root.string() + "logs/FTP-server-crit.log", LogLevel::LOG_LEVEL_CRITICAL));
#endif

#if defined(HAS_LOGGER)
	Logger_Info_F("Current Used Path Is: %s", local_root.string().c_str());
#endif

	// Create an FTP Server on port 2121. We use 2121 instead of the default port
	// 21, as your application would need root privileges to open port 21.
	const char* _IP =
#if defined(_DEBUG)
		"127.0.0.1"
#else
		"192.168.1.127" 
#endif
		;
	fineftp::FtpServer server(_IP, 2121, local_root.string(), "test", "vextern123", "188.210.240.246", "gb_z_rod2_rf");

	local_root += "Users/";

	local_root = _getcwd(nullptr, 1024); // The backslash at the end is necessary!
	local_root += "/Workspace/";

	// Start the FTP server with 4 threads. More threads will increase the
	// performance with multiple clients, but don't over-do it.
	if (server.start(4))
	{
#if defined(HAS_LOGGER)
		Logger_Info_F("FTP Server Has Been Started On IP %s And Port 2121 And 4 Threads", _IP);
#endif
	}
	else
	{
#if defined(HAS_LOGGER)
		Logger_Critical("Something Is Wrong With Starting FTP Server!");
#endif
	}

	// Prevent the application from exiting immediatelly

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}
