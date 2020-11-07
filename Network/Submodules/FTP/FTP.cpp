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

#include <cryptlib.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <md5.h>
#include <files.h>
#include <hex.h>
#include <Shellapi.h>

#define PORT 20675
#define IP swl::IPEndpoint("192.168.1.2")
std::shared_ptr<mysql::MYSQLCLIENT> User = std::make_shared<mysql::MYSQLCLIENT>();
std::shared_ptr<net::Client> Client = std::make_shared<net::Client>();
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

	return strHash;
}

bool addAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	auto AuthorList = User->TrySelectValues("user_wright", { "Username", username });

	for (auto ThisRow : AuthorList)
	{
		if (ThisRow.second["_0"] == authorname)
		{
			if (ThisRow.second["_1"].is_null() || (ThisRow.second["_1"].is_string() &&
				ThisRow.second["_1"].get<json::string_t>().empty()))
				ThisRow.second["_1"] = {};
			else if ((ThisRow.second["_1"].is_string() && !ThisRow.second["_1"].get<json::string_t>().empty()))
				ThisRow.second["_1"] = json::parse(ThisRow.second["_1"].get<json::string_t>());
			ThisRow.second["_1"].push_back(filename);
			std::cerr << " " << ThisRow.second.dump() << " ";
			User->TryInsertValues("user_wright", { username }, { ThisRow.second["_1"].dump() });
			return false;
		}
	}
	return true;
}

unsigned remove(nlohmann::json& jsonObject, const std::string& value)
{
	std::vector<int> toremove;
	for (auto &it: jsonObject.items()) {
		if (it.value().get<std::string>() == value)
			toremove.push_back(stoi(it.key()));
	}
	std::sort(toremove.rbegin(), toremove.rend());
	for (int &it : toremove)
		jsonObject.erase(jsonObject.begin() + it);
	return toremove.size();
}

bool removeAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	auto AuthorList = User->TrySelectValues("user_wright", { "Username", username });
	for (auto ThisRow : AuthorList)
	{
		if (ThisRow.second["_0"] == authorname)
		{
			OutputDebugStringA(ThisRow.second["_1"].dump().c_str());
			if ((ThisRow.second["_1"].is_string() && !ThisRow.second["_1"].get<json::string_t>().empty()))
				ThisRow.second["_1"] = json::parse(ThisRow.second["_1"].get<json::string_t>());

			remove(ThisRow.second["_1"], filename);

			User->TryInsertValues("user_wright", { username }, { ThisRow.second["_1"].dump() }); // seems to be same as previous issue (f*ck json)
			return false;
		}
	}
	return true;
}

bool hasUserAccess(const std::string& filename, const std::string& authorname, const std::string& username)
{
	if (username == authorname)
	{
		path authorpath = _getcwd(nullptr, 1024); // Uncomment only on real server, this is for checking for file existence
		authorpath += "\\" + authorname + "\\" + filename;
		ifstream targetfile{ authorpath };
		if (targetfile.is_open())
			return true; // file is in author folder + has access
		else
			return false; //no file in author folder
	}

	auto UserAccess = User->TrySelectValues("user_wright", { "Username", username }, { "Username = '" + authorname + "'" });

	if (UserAccess.size() == 0)
		return false;

	for (auto ThisAccess : UserAccess)
	{
		if (ThisAccess.second["_1"].dump().find(filename) != std::string::npos)
		{
			path authorpath = _getcwd(nullptr, 1024); //Uncomment only on real server, this is for checking for file existence
			authorpath += "\\" + authorname + "\\" + filename;
			ifstream targetfile{ authorpath };
			if (targetfile.is_open())
				return true; // file is in author folder + has access
			else
				return false; //no file in author folder
			return true;
		}
		else
			return false; // no access
	}
	return false; //no author
}

int main()
{
	//User->Connect("gb_z_rod2_rf", "696ea7b8ty", "mysql101.1gb.ru", "gb_z_rod2_rf");

	User->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
		"188.210.240.246"
#else
		"192.168.1.2"
#endif
		, "gb_z_rod2_rf");

	//std::cerr << hasUserAccess("doc.doc", "user2", "user1");
	//std::cerr << addAccess("text1.text", "user1", "user2");
	//std::cerr << hasUserAccess("text.text", "user1", "user2");
	//std::cerr << removeAccess("text1.text", "user1", "user2");
	//std::cerr << hasUserAccess("text.text", "user1", "user2");

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

	auto AllUsers = User->TrySelectValues("Local", { "*" });

	local_root += "Users/";
	for (auto ThisUser: AllUsers)
	{
		auto ThisPath = local_root.string() + ThisUser.second["_0"].get<std::string>();
		if (!exists(ThisPath))
			create_directories(ThisPath);
		if (ThisUser.second["_3"].get<int>() == 1)
			server.addUser(ThisUser.second["_0"], ThisUser.second["_1"], local_root.string(), fineftp::Permission::All);
		else
			server.addUser(ThisUser.second["_0"], ThisUser.second["_1"], ThisPath, fineftp::Permission::FileWrite);
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

		// Check For Updates
		if (!exists(local_root.string() + "updates"))
			create_directories(local_root.string() + "updates/");

		int cnt = std::count_if(
			directory_iterator(local_root.string() + "updates/"),
			directory_iterator(),
			static_cast<bool(*)(const path&)>(is_regular_file));
		if (cnt > 1)
		{
			if (Programm.empty() && Hash2.empty())
			{
				for (directory_iterator it(local_root.string() + "updates/"); it != directory_iterator(); ++it)
				{
					if (it->path().filename() != "update_tcp.exe")
						Hash2 = it->path().filename().string();
					else
						Programm = it->path();
				}
			}
			else if (Client->IsRunning() && Client->GetConnect() && (!Programm.empty() && !is_empty(Programm)))
			{
				swl::Packet packet = swl::Packet();
				Client->GetConnect()->GetPacket(packet, (swl::Packet::Type)
					(swl::Packet::Type::Answer << swl::Packet::Type::ClosedServerByUpdate));
				if (packet)
				{
					local_root = _getcwd(nullptr, 1024);
					local_root += "/Workspace/";
					Hash2 += ".exe";

					// Call New Server
					if (!exists(local_root.string() + Hash2))
						boost::filesystem::copy_file(Programm, local_root.string() + Hash2);

					boost::filesystem::remove_all(local_root.string() + "updates");
					ShellExecuteA(0, "open", (local_root.string() + Hash2).c_str(), 0, 0, 1);
					Programm.clear();
					Hash2.clear();

					Client->Disconnect();
					Client->StopSystem();
				}
			}

			else if (!Client->GetConnect() || !Client->IsRunning() && !Programm.empty() && !is_empty(Programm))
			{
				auto Hash1 = md5_from_file(Programm.string());
				if ((!Hash1.empty() && !Hash2.empty()) && Hash2 == Hash1)
				{
					// Connecting to server and send ClosedServerByUpdate Packet
					Client->Connect(IP, PORT);
					Client->StartSystem();

					swl::Packet packet = swl::Packet();
					json pack = packet.CreateMessage();
					packet.FillIn(swl::Packet::Header(swl::Packet::Type::ClosedServerByUpdate), pack);
					Client->GetConnect()->Send(packet);
				}
			}
		}
	}

	return 0;
}
