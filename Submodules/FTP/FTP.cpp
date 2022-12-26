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
#include "Servers.hpp"

std::shared_ptr<network::ServerFTP> This_Server_FTP;
std::shared_ptr<network::ClientFTP> This_Client;

int main()
{
	setlocale(LC_ALL, "Russian");

	std::string EnteredOption;
	for (;;)
	{
		std::cout << "Enter the one of option:\ns - for server\nc - for client\n\n";
		std::cin >> EnteredOption;
		for (;;)
		{
			if (EnteredOption == "s")
			{
				EnteredOption = "";
				if (This_Server_FTP)
				{
					This_Server_FTP->StopSystem();
				}
				This_Server_FTP = std::make_shared<network::ServerFTP>();
				This_Server_FTP->Start();
				//CppLogger::DisablePrintAll();
				//Logger_Error("Now It Doesn't Work");
			}
			else if (EnteredOption == "c")
			{
				EnteredOption = "";
				This_Client = std::make_shared<network::ClientFTP>();
				if (This_Client->Connect("127.0.0.1", "anonym", "", 21))
				{
					std::filesystem::path CurrentWorkingFile = _getcwd(nullptr, 0);
					This_Client->SendFile(CurrentWorkingFile.string() + "\\test_database.mdb");

					for (size_t i = 0; i < 5000; i++)
					{
						This_Client->SendFile(CurrentWorkingFile.string() + "\\test_database.mdb");
					}
					//CppLogger::EnablePrintAll();
					//Logger_Error("Now It Works");
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	return 0;
}
