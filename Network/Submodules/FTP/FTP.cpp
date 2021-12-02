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

typedef std::pair<std::string, short> FileRights;

std::shared_ptr<mysql::Client> mysqlDB = std::make_shared<mysql::Client>();
std::shared_ptr<odbc::ODBC> odbcDB = std::make_shared<odbc::ODBC>();

int main()
{
	setlocale(LC_ALL, "Russian");

	/*odbcDB->Connect("Microsoft Access Driver (*.mdb)", "F:\\Programming\\C++\\Project\\ODBC\\ODBC\\test.MDB", "READONLY=false", "12345");
	odbc::addUser("user4", {}, 0);
	odbc::addUser("user6", {}, 0);
	odbc::updateFilesRights("user6", { {"doc.doc", 3} });
	std::cerr << odbc::hasUserAccessToFile("user6", "doc.doc", 1);
	odbc::updateFilesRights("user3", { {"text.text", 1}, {"mp4.mp4", 2} });
	std::cerr << odbc::hasUserAccessToFile("user3", "text.text", 1) << odbc::hasUserAccessToFile("user3", "mp4.mp4", 1);
	odbc::updateFilesRights("user3", { {"mp4.mp4", 0} });
	std::cout << odbc::hasUserAccessToProject("user1", 1);*/
	//db.SplitDB("New", "G:/DecisionSolver/Engine/Workspace/resource/New.mdb");

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

	/*if (mysqlDB->Connect("test", "vextern123",
#if defined(_DEBUG)
		"188.210.240.246"
#else
		"192.168.1.2"
#endif
		, "gb_z_rod2_rf") == mysql::Client::Done)
	{
#if defined(_DEBUG) && defined(HAS_LOGGER)
		const char* _IP =
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"192.168.1.2"
#endif
			;
		Logger_Debug_F("Successful Connected To %s", _IP);
#endif
#if defined(HAS_LOGGER)

		Logger_Info("Successful Connect To MySQL DB");
#endif
	}
	else
	{
#if defined(_DEBUG) && defined(HAS_LOGGER)
		const char* _IP =
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"192.168.1.2"
#endif
			;
		Logger_Debug_F("Failure Connected To %s", _IP);
#endif
#if defined(HAS_LOGGER)
		Logger_Info("Failure Connect To MySQL DB");
#endif
		return -1;
	}*/

	/*mysql::addUser("user1", {}, 0);
	mysql::addUser("user3", {}, 0);
	mysql::updateFilesRights("user1", { {"doc.doc", 0} });
	std::cerr << mysql::hasUserAccessToFile("user1", "doc.doc", 1);
	mysql::updateFilesRights("user3", { {"text.text", 1}, {"mp4.mp4", 2} });
	std::cerr << mysql::hasUserAccessToFile("user3", "text.text", 1) << mysql::hasUserAccessToFile("user3", "mp4.mp4", 1);
	mysql::updateFilesRights("user3", { {"mp4.mp4", 0} });
	std::cout << mysql::hasUserAccessToProject("user1", 1);*/

#if defined(HAS_LOGGER)
	Logger_Info_F("Current Used Path Is: %s", local_root.string().c_str());
#endif

	// Create an FTP Server on port 2121. We use 2121 instead of the default port
	// 21, as your application would need root privileges to open port 21.
	const char* _IP =
#if defined(_DEBUG)
		"127.0.0.1"
#else
		"192.168.1.2"
#endif
		;
	//fineftp::FtpServer server(_IP, 2121, local_root.string(), "test", "vextern123", "188.210.240.246", "gb_z_rod2_rf");
	//fineftp::FtpServer server(_IP, 2121, local_root.string(), "Microsoft Access Driver (*.mdb)", "F:\\Programming\\C++\\Project\\ODBC\\ODBC\\test.MDB", std::vector<std::string>({ "READONLY=false" }), "12345");
	
	// Add the well known anonymous user and some normal users. The anonymous user
	// can log in with username "anonyous" or "ftp" and any password. The normal
	// users have to provide their username and password. 

	//auto AllUsers = mysqlDB->SelectValues("Local", { "*" });

	local_root += "Users/";

	/*for (size_t i = 0; i < AllUsers["_N"].size(); i++)
	{
		auto ThisPath = local_root.string() + AllUsers["_0"].at(i).get<std::string>();
		if (!exists(ThisPath))
			create_directories(ThisPath);
		if (AllUsers["_3"].at(i).get<json::number_integer_t>() == 1)
			server.addUser(AllUsers["_0"].at(i), AllUsers["_1"].at(i), fineftp::Permission::All);
		else
			server.addUser(AllUsers["_0"].at(i), AllUsers["_1"].at(i), fineftp::Permission::FileWrite);
	}*/
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
