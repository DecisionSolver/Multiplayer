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

	bool SendFile(const boost::filesystem::path &FilePath);
	bool ReceiveFile(const boost::filesystem::path &Path, const boost::filesystem::path &Where);

	const uint16_t &getPort();
	const std::string &getIP();

	bool IsConnected() { return Connected; }
private:
	const uint16_t port_ = 2121;
	const std::string IP, UserName;
	CURL *curl = nullptr;

	struct WriteThis {
		std::stringstream readptr;
		size_t sizeleft = 0u;
	};
	struct FtpFile {
		std::string filename;
		FILE *stream = nullptr;
	};
	static size_t write_callback(void *buffer, size_t size, size_t nmemb, void *stream);
	static size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *stream);
	static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

	const bool Connected = false;
};