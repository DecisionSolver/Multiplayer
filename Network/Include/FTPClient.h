#pragma once
#include "pch.h"

#include <curl/curl.h>
#include <Boost/filesystem.hpp>

namespace network
{
	class ClientFTP
	{
	public:
		ClientFTP();
		~ClientFTP();

		bool Connect(const std::string &ServerIP, const std::string &Login, const std::string &Pass, const uint16_t &Port = 0);
		void Disconnect();

		bool SendFile(const std::filesystem::path &FilePath);
		bool ReceiveFile(const std::filesystem::path &FilenameFromFTP_Folder, const std::filesystem::path &Where_FullPath);

		const uint16_t &getPort();
		const std::string &getIP();

		bool IsConnected() { return Connected; }
	private:
		std::string Stored_IP, Stored_Username, Stored_Password;
		uint16_t Stored_Port = 0;
		CURL *curl = nullptr;

		struct WriteData
		{
			std::stringstream Read_Buffer;
			size_t Size_Left = 0u;
		};
		struct FtpFile
		{
			std::string file_path;
			FILE *stream = nullptr;
		};
		static size_t Write(void *Buffer, size_t Size, size_t nmemb, void *stream);
		static size_t getContentLength(void *Buffer, size_t Size, size_t nmemb, void *stream);
		static size_t Read(void *Buffer, size_t Size, size_t nmemb, void *stream);

		bool Connected = false;
	};
}