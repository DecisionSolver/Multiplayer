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

namespace tests
{
	void MySQLTest()
	{
		Logger_Info("Testing MySQL API\n");

		std::string user = "",
			password     = "",
			ip           = ""; //add data to log in DB

		mysql::Client client;
		if (client.Connect(user, password, ip, "test_db", "") != mysql::Client::Status::Done)
		{
			Logger_Warn("Failed to connect to Test DB. Aborting tests\n");
			return;
		}

		try
		{
			client.CreateTable("Test_Table", { "N", "Test1" }, { "INT", "TEXT" }, { "", "" }, { {}, {} });
			client.SetCurrentTable("Test_Table");

			client.CreateColumn("", "Test2", "INT", "", { {} });
			client.InsertValues("", { "N", "Test1", "Test2" }, { "5", "TestTestTest", "42" });
			std::cerr << client.SelectValues("", { "N", "Test1", "Test2" }) << std::endl;
			client.UpdateValues("", { "Test1" }, { "Empty" }, { "`N` = '5'" });
			std::cerr << client.SelectValues("", { "N", "Test1", "Test2" }, { "`N` = '5'" }) << std::endl;
			client.DeleteColumn("", "Test2");
			std::cerr << client.SelectValues("", { "*" }, {}) << std::endl;
			client.DeleteTable("");

			Logger_Info("All tests passed succesfully\n");
		}
		catch (sql::SQLException& e)
		{
			Logger_Warn_F("Got exception {} on one of the tests : {}. Aborting tests\n", e.getErrorCode(), e.what());
			client.DeleteTable("Test_Table");
		}

		client.Destroy();
	}

	void ODBCTest()
	{
		Logger_Info("Testing ODBC API\n");

		odbc::ODBC client;
		std::string path = ""; //add path to DB

		if (!client.Connect("Microsoft Access Driver (*.mdb)", path, "", { "READONLY=false" }, "12345"))
		{
			Logger_Warn("Failed to connect to Test DB. Aborting tests\n");
			return;
		}

		client.CreateTable("Test_Table", { "N", "Test1" }, { "INTEGER", "VARCHAR" }, { "", "" }, { {}, {} });
		client.SetCurrentTable("Test_Table");

		client.CreateColumn("", "Test2", "INTEGER", "", { {} });
		client.InsertValues("", { "N", "Test1", "Test2" }, { "5", "TestTestTest", "42" });
		std::cerr << client.SelectValues("", { "N", "Test1", "Test2" }) << std::endl;
		client.UpdateValues("", { "Test1" }, { "Empty" }, { "`N` = 5" });
		std::cerr << client.SelectValues("", { "N", "Test1", "Test2" }, { "`N` = 5" }) << std::endl;
		client.DeleteColumn("", "Test2");
		std::cerr << client.SelectValues("", { "*" }, {}) << std::endl;
		client.DeleteTable("");

		Logger_Info("If there's no messages, then all tests passed\n");

		client.Exit();
	}
}

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
