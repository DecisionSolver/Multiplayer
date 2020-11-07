#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include <conio.h>

using namespace std;
#include "MySQL/MySQL_Client.h"
#include "MySQL/MySQL_Impl.h"

std::shared_ptr<mysql::Impl> DB = std::make_shared<mysql::Impl>();

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	DB->Connect("7f5acfc6", "c21d854c6d3b7a9b0d4c3bf52f0b9af6caffa8fd",
#if defined(_DEBUG)
		"188.210.240.246"
#else
		"192.168.1.2"
#endif
		, "gb_z_rod2_rf");
	DB->InsertValues("user_wright", { "Authorname" }, { "TEST" });

	return 0;
}
