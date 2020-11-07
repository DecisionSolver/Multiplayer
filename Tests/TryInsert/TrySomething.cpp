#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include <conio.h>

using namespace std;

#include <stdio.h>
#include <ftpclient.h>
#include <direct.h>

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

	FTPClient cl = FTPClient();
	if (cl.Connect("192.168.121.1", "Uploader", "123456" /*"PBAX", "OK"*/))
	{
		std::string path = _getcwd(nullptr, 1024);
		cl.ReceiveFile("AutoRun.InF", (path + "/NewFile.txt"));
		cl.SendFile("G:/AutoRun.InF");
	}
	cl.Disconnect();
	return 0;
}
