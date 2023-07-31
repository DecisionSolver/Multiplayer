#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "fineftp/server.h"

#include "ODBC/ODBC.h"
#include "Servers.hpp"

#include "Project System/File_system.h"

#include <filesystem>
#include <fstream>

#include "user_database.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <stdlib.h>
#include <iostream>

#include "mysql connector/include/jdbc/mysql_connection.h"

#include <mysql connector/include/jdbc/cppconn/driver.h>
#include <mysql connector/include/jdbc/cppconn/exception.h>
#include <mysql connector/include/jdbc/cppconn/resultset.h>
#include <mysql connector/include/jdbc/cppconn/statement.h>

#include <SocketTCP.h>

using namespace std::chrono_literals;

#ifdef CreateFile
	#undef CreateFile
#endif // CreateFile

extern std::shared_ptr<File_system> FS;
std::mutex MutexForTesting;

void CreateFile(const std::filesystem::path &Filename, const std::string &DataToWrite)
{
	std::ofstream outFile(Filename);

	outFile << DataToWrite << std::endl;

	outFile.close();
}

void UpdateUserPermission(std::shared_ptr<mysql::Client> client, std::shared_ptr<fineftp::FtpServer> ServerFTP, int PermissionToUpdate)
{
	// Update User's File Permission
	client->UpdateValuesInCurrentTable({ "FilesPermissions" }, { "{ \".AllFolderFiles\":" +
		std::to_string(PermissionToUpdate) + "}" },
		{ "Username = 'anonym'" });

	ServerFTP->GetUsers()->checkOrUpdatePermissions("anonym");
}

/*
TEST(TestingDataBases, MySQL)
{
	std::cout << "------Testing MySQL API" << std::endl << std::endl;

	std::string user = "root",
		password = "",
		ip = "localhost";

	std::shared_ptr<mysql::Client> client = std::make_shared<mysql::Client>();

	ASSERT_TRUE(client->Connect(user, password, ip) != mysql::Client::Status::Error);

	client->DeleteDatabase("test_db");

	client->CreateDatabaseAndSetCurrent("test_db");
	EXPECT_STREQ(client->GetCurrentDatabase().c_str(), "test_db");

	client->CreateTable("Test_Table", { "N", "Test1" }, { "INT", "TEXT" }, { "", "" }, { {}, {} });

	client->SetCurrentTable("Test_Table");
	
	EXPECT_STREQ(client->GetCurrentTable().c_str(), "Test_Table");
	
	client->CreateColumnInCurrentTable("Test2", "INT", "", { {} });
	client->InsertValuesInCurrentTable({ "N", "Test1", "Test2" }, { "5", "TestTestTest", "42" });

	nlohmann::json Result = client->SelectValuesInCurrentTable({ "N", "Test1", "Test2" });
	std::cout << Result.dump() << std::endl;

	ASSERT_TRUE(Result["N"][0] == 5);
	ASSERT_TRUE(Result["Test2"][0] == 42);
	EXPECT_STREQ(Result["Test1"][0].get<nlohmann::json::string_t>().c_str(), "TestTestTest");

	client->UpdateValuesInCurrentTable({ "Test1" }, { "Empty" }, { "`N` = '5'" });

	Result = client->SelectValuesInCurrentTable({ "N", "Test1", "Test2" }, { "`N` = '5'" });
	std::cout << Result.dump() << std::endl;
	
	ASSERT_TRUE(Result["N"][0] == 5);
	ASSERT_TRUE(Result["Test2"][0] == 42);
	EXPECT_STREQ(Result["Test1"][0].get<nlohmann::json::string_t>().c_str(), "Empty");

	client->DeleteColumnInCurrentTable("Test2");

	Result = client->SelectValuesInCurrentTable({ "*" }, {});
	std::cout << Result.dump() << std::endl;
	
	ASSERT_TRUE(Result["N"][0] == 5);
	EXPECT_STREQ(Result["Test1"][0].get<nlohmann::json::string_t>().c_str(), "Empty");
	
	client->DeleteCurrentTable();

	// will show the error because previous call delete the DB and also the Table
	//client->DeleteTable("Test_Table");
}

TEST(TestingDataBases, ODBC)
{
	std::cout << "------Testing ODBC API" << std::endl << std::endl;

	odbc::ODBC client;
	std::vector<std::string> DatabaseAttributes = { "READONLY=false" };
	std::string Password = "12345";
	nlohmann::json Result;

	std::filesystem::path CurrentWorkingFile = _getcwd(nullptr, 0);
	CurrentWorkingFile += "\\test_database.mdb";

	if (std::filesystem::exists(CurrentWorkingFile))
	{
		std::filesystem::remove(CurrentWorkingFile);
	}

	client.CreateDataBase(client.DefaultDriverString, CurrentWorkingFile.string(), DatabaseAttributes.back(), Password);

	ASSERT_TRUE(client.Connect(client.DefaultDriverString, CurrentWorkingFile.string(), "", DatabaseAttributes, Password));

	client.CreateAndSetCurrentTable("Test_Table", { "ColumnNumber", "ColumnString" }, { "INTEGER", "VARCHAR" }, { "", "" }, { {}, {} });

	client.CreateColumnInCurrentTable("AnotherColumnNumber", "INTEGER", "", { {} });

	client.InsertValuesInCurrentTable({ "ColumnNumber", "ColumnString", "AnotherColumnNumber" }, { "5", "Test1Test2Test3", "42" });
	
	Result = client.SelectValuesInCurrentTable({ "ColumnNumber", "ColumnString", "AnotherColumnNumber"});
	ASSERT_TRUE(Result["AnotherColumnNumber"][0] == 42);
	ASSERT_TRUE(Result["ColumnNumber"][0] == 5);
	EXPECT_STREQ(Result["ColumnString"][0].get<nlohmann::json::string_t>().c_str(), "Test1Test2Test3");

	// will throw an exception (wrong column)
	client.UpdateValuesInCurrentTable({ "Test2" }, {"Empty"}, {"`ColumnNumber` = 5"});

	client.UpdateValuesInCurrentTable({ "ColumnString" }, {"Empty"}, {"`ColumnNumber` = 5"});

	Result = client.SelectValuesInCurrentTable({ "ColumnNumber", "ColumnString", "AnotherColumnNumber" }, { "`ColumnNumber` = 5" });
	ASSERT_TRUE(Result["AnotherColumnNumber"][0] == 42);
	ASSERT_TRUE(Result["ColumnNumber"][0] == 5);
	EXPECT_STREQ(Result["ColumnString"][0].get<nlohmann::json::string_t>().c_str(), "Empty");

	client.DeleteColumnInCurrentTable("AnotherColumnNumber");

	Result = client.SelectValuesInCurrentTable({ "*" }, {});
	ASSERT_TRUE(Result["ColumnNumber"][0] == 5);
	EXPECT_STREQ(Result["ColumnString"][0].get<nlohmann::json::string_t>().c_str(), "Empty");

	client.DeleteCurrentTable();

	client.Destroy();
}

TEST(TestingFTP, FTP_API)
{
	std::cout << "------Testing FTP API" << std::endl << std::endl;
	std::cout << "--------------Test Connect To Server That Doesn't Work!" << std::endl;

	std::shared_ptr<network::ServerFTP> This_Server_FTP = std::make_shared<network::ServerFTP>();
	std::shared_ptr<network::ClientFTP> This_Client = std::make_shared<network::ClientFTP>();

	ASSERT_FALSE(This_Client->Connect("127.0.0.1", "anonym", ""));

	std::cout << "--------------Create FTP Server!" << std::endl;

	std::thread([&]
	{
		This_Server_FTP->Start();
	}).detach();
	
	std::this_thread::sleep_for(std::chrono::seconds(5));

	// Disable Whole Permissions (That Means User Has Available To Do Anything)
	ASSERT_TRUE(This_Server_FTP->GetFTPServer()->addNewUser("anonym", "",
		fineftp::UserPermission::All, "{\".AllFolderFiles\":0}"_json));

	std::cout << "--------------Test Connect To Server With Wrong Credentials!" << std::endl;
	ASSERT_FALSE(This_Client->Connect("127.0.0.1", "reeerwerw", "423412312"));


	bool SuccessConnection = false;
	ASSERT_TRUE(SuccessConnection = This_Client->Connect("127.0.0.1", "anonym", ""));

	std::filesystem::path CurrentWorkingFile = _getcwd(nullptr, 0);
	CurrentWorkingFile += "\\Workspace\\test_file1.txt";

	// start the thread for File Watcher
	std::thread([&]
	{
		while (This_Server_FTP && This_Server_FTP->IsRunning())
		{
			FS->Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}).detach();
	FS->AddFolderToWatch(CurrentWorkingFile.parent_path().string(),
		[&](FW::WatchID watchid, const FW::String &dir, const FW::String &filename, FW::Action action)
	{
		std::cout << "WatchID: " << watchid << "\nDir: " << dir << "\nFilename: " << filename << std::endl;
		switch (action)
		{
			case FW::Action::Add:
			{
				std::cout << "Action: Action::Add" << std::endl << std::endl;
				break;
			}
			case FW::Action::Delete:
			{
				std::cout << "Action: Action::Delete" << std::endl << std::endl;
				break;
			}
			case FW::Action::Modified:
			{
				std::cout << "Action: Action::Modified" << std::endl << std::endl;
				break;
			}
		}
	});

	if (std::filesystem::exists(CurrentWorkingFile))
	{
		std::filesystem::remove(CurrentWorkingFile);
	}
	if (std::filesystem::exists(CurrentWorkingFile.parent_path().string() + "\\new_received_file.txt"))
	{
		std::filesystem::remove(CurrentWorkingFile.parent_path().string() + "\\new_received_file.txt");
	}

	CreateFile(CurrentWorkingFile, "Test File Data Is Here!");

	std::condition_variable IsReadyToGo;

	ASSERT_TRUE(This_Client->SendFile(CurrentWorkingFile));

	std::shared_ptr<mysql::Client> client = std::make_shared<mysql::Client>();
	{
		// wait for detached thread server when it start!
		std::unique_lock<std::mutex> IsReadyToGo_LockMutex(MutexForTesting);
		IsReadyToGo.wait_for(IsReadyToGo_LockMutex, 15s);

		std::string user = "root",
			password = "",
			ip = "localhost";

		ASSERT_TRUE(client->Connect(user, password, ip) != mysql::Client::Status::Error);

		client->SetCurrentTable("Users");

		// Disable Download Files
		UpdateUserPermission(client, This_Server_FTP->GetFTPServer(), (int)fineftp::FilePermission::FileRead);

		ASSERT_FALSE(This_Client->ReceiveFile(CurrentWorkingFile, CurrentWorkingFile.parent_path().string() + "\\cannot_download_this_file.txt"));

		// Allow Download Files
		UpdateUserPermission(client, This_Server_FTP->GetFTPServer(), (int)fineftp::FilePermission::None);

		ASSERT_TRUE(This_Client->ReceiveFile(CurrentWorkingFile, CurrentWorkingFile.parent_path().string() + "\\new_received_file.txt"));
	}

	std::thread([&]
	{
		// stress test
		for (size_t i = 0; i < 1000; i++)
		{
			ASSERT_TRUE(This_Client->SendFile(CurrentWorkingFile));
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}).join();

	This_Server_FTP->StopSystem();
}
*/

void StartTestServerTCP()
{
	asio::io_context io_context;
	asio::ip::tcp::acceptor acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 8080));

	while (true)
	{
		asio::ip::tcp::socket socket(io_context);
		acceptor.accept(socket);

		asio::streambuf buf;
		asio::read_until(socket, buf, "\n");

		std::string response = "There you are\n";
		asio::write(socket, asio::buffer(response));
	}
}

asio::io_context io_context;

void IoServiceThread()
{
	try
	{
		io_context.run();
		DWORD last_error = ::GetLastError();
		asio::error_code ec(last_error, asio::error::get_system_category());
		asio::detail::throw_error(ec, "IoServiceThreadProc");
	}
	catch (std::system_error &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("System error caught in io_service socket thread. Exception: {}\nError Code: {}\n",
			e.what(), e.code().value());
#endif
	}
	catch (std::exception &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Standard exception caught in io_service socket thread. Exception: {}\n", e.what());
#endif
	}
	catch (...)
	{
#if __has_include("logger.h")
		Logger_Error_F("Unhandled exception caught in io_service socket thread.\n");
#endif
	}
}

TEST(TestingReadWriteSockets, BasicOfSockets)
{
	std::thread([&]
	{
		IoServiceThread();
	}).detach();

	std::shared_ptr<SocketTCP> NewSocket_TCP = std::make_shared<SocketTCP>();
	ASSERT_TRUE(NewSocket_TCP->ConnectAsync("127.0.0.1", 5000, io_context));
	
	(*NewSocket_TCP) << "Hello World!";

	ASSERT_TRUE(NewSocket_TCP->SendAsync());

	ASSERT_TRUE(NewSocket_TCP->ReceiveAsync());
	
	asio::streambuf buffer;
	std::istream data(&buffer);
	(*NewSocket_TCP) >> data;

	ASSERT_TRUE(buffer.size() != 0);

	ASSERT_TRUE(std::string({ buffers_begin(buffer.data()),
		buffers_end(buffer.data()) }) != "There you are!");
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}