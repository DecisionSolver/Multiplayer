#pragma once
#include "pch.h"

#include <curl/curl.h>
#include <Boost/filesystem.hpp>

#if defined(IP)
	#undef IP
#endif

class FTPClient
{
public:
	FTPClient();
	~FTPClient();

	bool Connect(const std::string &ServerIP, const std::string &Login, const std::string &Pass, const uint16_t &Port = 0);
	void Disconnect();

	void SendFile(const boost::filesystem::path &FilePath);
	void ReceiveFile(const boost::filesystem::path &FilePath, const boost::filesystem::path &Where);

	uint16_t getPort();
	std::string getIP();
private:
	const uint16_t port_ = 2121;
	const std::string IP, UserName;
	CURL *curl = nullptr;
};