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

#include <cryptlib.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <md5.h>
#include <files.h>
#include <hex.h>
#include <Shellapi.h>

std::shared_ptr<mysql::Client> DB = std::make_shared<mysql::Client>();

path Programm;
std::string Hash2;

const std::string md5_from_file(const std::string& path)
{
	if (Programm.empty() && Hash2.empty()) return "";

	CryptoPP::Weak1::MD5 md;
	const size_t size = CryptoPP::Weak1::MD5::DIGESTSIZE * 2;
	CryptoPP::byte buf[size] = { 0 };
	CryptoPP::FileSource(
		path.c_str(), true,
		new CryptoPP::HashFilter(
			md, new CryptoPP::HexEncoder(new CryptoPP::ArraySink(buf, size))));
	std::string strHash = std::string(reinterpret_cast<const char*>(buf), size);
	std::cout << strHash.c_str() << std::endl;

	boost::to_upper(strHash);
	return strHash;
}

void addUser(const std::string& username)
{
	DB->CreateColumn("user_wright", username, "TEXT", "", {});
	DB->TryInsertValues("user_wright", { "Authorname" }, { username });
}

void addAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	auto AuthorList = DB->TrySelectValues("user_wright", { username }, { "Authorname = '" + authorname + "'" });

	if (!AuthorList["f"].is_array() || AuthorList["f"].empty())
		AuthorList.clear();

	if (AuthorList["f"].dump().find(filename) == std::string::npos)
	{
		AuthorList["f"].push_back(filename);
		DB->TryInsertValues("user_wright", { username }, { AuthorList.dump() }, { "Authorname = '" + authorname + "'" });
	}
}

void removeAccess(const std::string &filename, const std::string& authorname, const std::string& username)
{
	auto AuthorList = DB->TrySelectValues("user_wright", { username }, { "Authorname = '" + authorname + "'" });

	for(auto it = AuthorList["f"].begin(); it < AuthorList["f"].end(); it++)
		if (it.value() == filename)
		{
			AuthorList["f"].erase(it - AuthorList["f"].begin());
			break;
		}
		

	if(!AuthorList["f"].empty())
		DB->TryInsertValues("user_wright", { username }, { AuthorList.dump() }, { "Authorname = '" + authorname + "'" });
	else
		DB->TryInsertValues("user_wright", { username }, { "" }, { "Authorname = '" + authorname + "'" });
}

bool hasUserAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	if (username == authorname)
	{
		path authorpath = _getcwd(nullptr, 1024);
		authorpath += "\\" + authorname + "\\" + filename;
		ifstream targetfile{ authorpath };
		if (targetfile.is_open())
			return true; // file is in author folder + has access
		else
			return false; //no file in author folder
	}

	auto UserAccess = DB->TrySelectValues("user_wright", { username }, { "Authorname = '" + authorname + "'" });

	if (UserAccess["f"].empty())
		return false; //no author

	if (UserAccess["f"].dump().find(filename) != std::string::npos)
	{
		path authorpath = _getcwd(nullptr, 1024);
		authorpath += "\\" + authorname + "\\" + filename;
		ifstream targetfile{ authorpath };
		if (targetfile.is_open())
			return true; // file is in author folder + has access
		else
			return false; //no file in author folder
	}
	else
		return false; // no access
}


int main()
{
	setlocale(LC_ALL, "Russian");

	path local_root = _getcwd(nullptr, 1024); // The backslash at the end is necessary!

	local_root += "/";
	if (!exists(local_root.string() + "Workspace"))
		create_directories(local_root.string() + "Workspace/");
	local_root += "Workspace/";

	if (!exists(local_root.string() + "updates"))
		create_directories(local_root.string() + "updates/");

	local_root = local_root.generic_path();

#if __has_include("../SelfLogger/logger.h")
	if (!exists(local_root.string() + "logs"))
		create_directories(local_root.string() + "logs/");

	CppLogger::registerTarget(
		new FileLoggerTarget(local_root.string() + "logs/FTP-server-all.log", LogLevel::LOG_LEVEL_DEBUG));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root.string() + "logs/FTP-server-err.log", LogLevel::LOG_LEVEL_ERROR));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root.string() + "logs/FTP-server-crit.log", LogLevel::LOG_LEVEL_CRITICAL));
#endif

	if (DB->Connect("test", "vextern123",
#if defined(_DEBUG)
		"188.210.240.246"
#else
		"192.168.1.2"
#endif
		, "gb_z_rod2_rf") == mysql::Client::Done)
	{
#if defined(_DEBUG) && __has_include("../SelfLogger/logger.h")
		const char *_IP =
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"192.168.1.2"
#endif
			;
		Logger_Debug_F("Successful Connected To %s", _IP);
#endif
#if __has_include("../SelfLogger/logger.h")

		Logger_Info("Successful Connect To MySQL DB");
#endif
	}
	else
	{
#if defined(_DEBUG) && __has_include("../SelfLogger/logger.h")
		const char *_IP =
#if defined(_DEBUG)
			"188.210.240.246"
#else
			"192.168.1.2"
#endif
			;
		Logger_Debug_F("Failure Connected To %s", _IP);
#endif
#if __has_include("../SelfLogger/logger.h")
		Logger_Info("Failure Connect To MySQL DB");
#endif
		return -1;
	}

	/*addAccess("doc.doc", "user2", "user1");
	std::cerr << hasUserAccess("doc.doc", "user2", "user1");
	addAccess("text.text", "user1", "user3");
	std::cerr << hasUserAccess("text.text", "user1", "user3");
	addAccess("text.text", "user3", "user1");
	std::cerr << hasUserAccess("text.text", "user3", "user1");
	removeAccess({ "text1.text" }, "user1", "user2");*/

#if __has_include("../SelfLogger/logger.h")
	Logger_Info_F("Current Used Path Is: %s", local_root.string().c_str());
#endif

	// Create an FTP Server on port 2121. We use 2121 instead of the default port
	// 21, as your application would need root privileges to open port 21.
	const char *_IP =
#if defined(_DEBUG)
		"127.0.0.1"
#else
		"192.168.1.2"
#endif
		;
	fineftp::FtpServer server(_IP, 2121);

	// Add the well known anonymous user and some normal users. The anonymous user
	// can log in with username "anonyous" or "ftp" and any password. The normal
	// users have to provide their username and password. 

	auto AllUsers = DB->TrySelectValues("Local", { "*" });

	local_root += "Users/";

	for (size_t i = 0; i < AllUsers["_N"].size(); i++)
	{
		auto ThisPath = local_root.string() + AllUsers["_0"].at(i).get<std::string>();
		if (!exists(ThisPath))
			create_directories(ThisPath);
		if (AllUsers["_3"].at(i).get<json::number_integer_t>() == 1)
			server.addUser(AllUsers["_0"].at(i), AllUsers["_1"].at(i), (_getcwd(nullptr, 1024) +
				std::string("/Workspace")), fineftp::Permission::All);
		else
			server.addUser(AllUsers["_0"].at(i), AllUsers["_1"].at(i), ThisPath, fineftp::Permission::FileWrite);
	}
	local_root = _getcwd(nullptr, 1024); // The backslash at the end is necessary!
	local_root += "/Workspace/";

	// Start the FTP server with 4 threads. More threads will increase the
	// performance with multiple clients, but don't over-do it.
	if (server.start(4))
	{
#if __has_include("../SelfLogger/logger.h")
		Logger_Info_F("FTP Server Has Been Started On IP %s And Port 2121 And 4 Threads", _IP);
#endif
	}
	else
	{
#if __has_include("../SelfLogger/logger.h")
		Logger_Critical("Something Is Wrong With Starting FTP Server!");
#endif
	}

	// Prevent the application from exiting immediatelly
	std::thread([&]
	{
		std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> oss;
		while (true)
		{
#if __has_include("../SelfLogger/logger.h")
			std::cout.rdbuf(oss.rdbuf());
			if (!oss.str().empty())
			{
				Logger_Info_F("FTP Server: %s", oss.str().c_str());
				oss.str("");
			}
#endif
		}
	}
	).detach();

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}
