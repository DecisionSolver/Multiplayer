#include <fineftp/server.h>

#include <iostream>
#include <thread>
#include <string>

#include "MySQL/MySQL_Client.h"
#include "MySQL/MySQL_Impl.h"

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

std::shared_ptr<mysql::Impl> DB = std::make_shared<mysql::Impl>();

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
	DB->InsertValues("user_wright", { "Authorname" }, { username });
}

void addAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	auto AuthorList = DB->TrySelectValues("user_wright", { username }, { "Authorname = '" + authorname + "'" });
	auto Row = AuthorList.begin();

	//if (Row->second["_0"].is_null() || (Row->second["_0"].is_string() && Row->second["_0"].get<json::string_t>().empty()))
	//	Row->second["_0"] = {};
	//else if ((Row->second["_0"].is_string() && !Row->second["_0"].get<json::string_t>().empty()))
	//	Row->second["_0"] = json::parse(Row->second["_0"].get<json::string_t>());

	//if (Row->second["_0"].dump().find(filename) == std::string::npos)
	//{
	//	Row->second["_0"].push_back(filename);
	//	DB->TryInsertValues("user_wright", { username }, { Row->second["_0"].dump() }, { "Authorname = '" + authorname + "'" });
	//}
}

void removeAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	auto AuthorList = DB->TrySelectValues("user_wright", { username }, { "Authorname = '" + authorname + "'" });
	auto Row = AuthorList.begin();

	//OutputDebugStringA(Row->second["_0"].dump().c_str());
	
	//if (Row.key() != "\"\"")
		//Row.key().(Row.key().find(filename), Row.key().find(filename) + filename.size());

	//DB->TryInsertValues("user_wright", { username },
	//	{ Row.key().dump() != "\"\"" ? Row.key().dump() : "" }, { "Authorname = '" + authorname + "'" });
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

	auto UserAccess = DB->TrySelectValues("user_wright", { "*" });

	if (UserAccess.size() == 0)
		return false; //no author

	if (UserAccess.begin().key().find(filename) != std::string::npos)
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
	DB->Connect("test", "vextern123",
#if defined(_DEBUG)
		"188.210.240.246"
#else
		"192.168.1.2"
#endif
		, "gb_z_rod2_rf");
//	addUser("user3");
	std::cerr << hasUserAccess("doc.doc", "user2", "user1");
	addAccess("text.text", "user1", "user3");
	std::cerr << hasUserAccess("text.text", "user1", "user3");
	addAccess("text.text", "user3", "user1");
	std::cerr << hasUserAccess("text.text", "user3", "user1");
	removeAccess("text1.text", "user1", "user2");

	path local_root = _getcwd(nullptr, 1024); // The backslash at the end is necessary!
	local_root += "/";
	if (!exists(local_root.string() + "Workspace"))
		create_directories(local_root.string() + "Workspace/");
	local_root += "/Workspace/";

	if (!exists(local_root.string() + "updates"))
		create_directories(local_root.string() + "updates/");

	// Create an FTP Server on port 2121. We use 2121 instead of the default port
	// 21, as your application would need root privileges to open port 21.
	fineftp::FtpServer server(2121);

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
				std::string("Workspace")), fineftp::Permission::All);
		else
			server.addUser(AllUsers["_0"].at(i), AllUsers["_1"].at(i), ThisPath, fineftp::Permission::FileWrite);
	}
	local_root = _getcwd(nullptr, 1024); // The backslash at the end is necessary!
	local_root += "/Workspace/";

	// Start the FTP server with 4 threads. More threads will increase the
	// performance with multiple clients, but don't over-do it.
	server.start(4);

	// Prevent the application from exiting immediatelly
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}
