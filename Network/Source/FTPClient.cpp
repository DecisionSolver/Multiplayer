#include "FTPClient.h"

size_t network::ClientFTP::getContentLength(void *Buffer, size_t Size, size_t nmemb, void *stream)
{
	long lengh = 0;
	int ErrorCode = sscanf_s((LPCSTR)Buffer, "Content-Length: %ld\n", &lengh);
	if (ErrorCode)
	{
		*((long *)stream) = lengh;
	}
	return Size * nmemb;
}

size_t network::ClientFTP::Write(void *Buffer, size_t Size, size_t nmemb, void *stream)
{
	struct FtpFile *out = (FtpFile *)stream;
	if (!out->stream && !out->file_path.empty())
	{
		if (fopen_s(&out->stream, out->file_path.c_str(), "wb") != 0 || !out->stream)
		{
			return 1; /* failure, can't open file to write */
		}
	}
	if (out->stream)
	{
		return fwrite(Buffer, Size, nmemb, out->stream);
	}
	else
	{
		return Size * nmemb;
	}
}
size_t network::ClientFTP::Read(void *Buffer, size_t Size, size_t nmemb, void *stream)
{
	FILE *File = (FILE *)stream;
	if (ferror(File))
	{
		return CURL_READFUNC_ABORT;
	}

	return fread(Buffer, Size, nmemb, File) * Size;
}

network::ClientFTP::ClientFTP()
{
	// Init Winsock
	CURLcode ErrorCode = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (ErrorCode != CURLE_OK)
	{
#if __has_include("logger.h")
		Logger_Error_F("curl_global_init() failed: {}\n", curl_easy_strerror(ErrorCode));
#endif
	}
	// Create And Init CURL
	curl = curl_easy_init();

	// Bind Upload And Download Function
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, Read);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Write);

#if defined(USE_SSL)
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
	curl_easy_setopt(curl, CURLOPT_CAINFO, "./ca.cert");
#endif

	// Enable Upload
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);

#if defined(_DEBUG)
	// Enable Full-log Message
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

}
network::ClientFTP::~ClientFTP()
{
	if (curl)
	{
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}

bool network::ClientFTP::SendFile(const std::filesystem::path &FilePath)
{
	if (!Connected)
	{
#if __has_include("logger.h")
		Logger_Warn("You're not connected to the FTP Server!");
#endif
		return false;
	}
	FILE *File;
	long uploaded_len = 0;
	CURLcode ResultCode = CURLE_GOT_NOTHING;
	int ErrorCode = 0;

	if (fopen_s(&File, FilePath.string().c_str(), "rb") != 0 || !File)
	{
#if __has_include("logger.h")
		Logger_Error_F("File \"{}\" Can Not Be Opened!", FilePath.string());
#endif

		return false;
	}

	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, ("ftp://" + Stored_IP + "/Users/" + Stored_Username +
		"/" + FilePath.filename().string()).c_str());
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getContentLength);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &uploaded_len);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)uploaded_len);

	curl_easy_setopt(curl, CURLOPT_READDATA, File);
	for (ErrorCode = 0; (ResultCode != CURLE_OK) && (ErrorCode < 1); ErrorCode++)
	{
		if (ErrorCode)
		{
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
			curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

			ResultCode = curl_easy_perform(curl);
			if (ResultCode != CURLE_OK)
			{
				continue;
			}
			curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

			fseek(File, uploaded_len, SEEK_SET);

			curl_easy_setopt(curl, CURLOPT_APPEND, 1L);
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_APPEND, 0L);
		}

		ResultCode = curl_easy_perform(curl);

		// May be needs to connect to again!
		if (ResultCode == CURLE_LOGIN_DENIED)
		{
			if (Connect(Stored_IP, Stored_Username, Stored_Password, Stored_Port))
			{
				ResultCode = CURLE_GOT_NOTHING;
			}
		}
	}

	if (File)
	{
		fclose(File);
	}

	if (ResultCode != CURLE_OK && ResultCode != CURLE_PARTIAL_FILE)
	{
#if __has_include("logger.h")
		Logger_Error_F("SendFile() failed: {}\n", curl_easy_strerror(ResultCode));
#endif
		return false;
	}
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
	return true;
}

bool network::ClientFTP::ReceiveFile(const std::filesystem::path &FilenameFromFTP_Folder, const std::filesystem::path &Where_FullPath)
{
	if (!Connected)
	{
#if __has_include("logger.h")
		Logger_Warn("You're not connected to the FTP Server!");
#endif
		return false;
	}
	FtpFile ftpfile =
	{
	   "",
	   nullptr
	};
	ftpfile.file_path = Where_FullPath.string();

	curl_easy_setopt(curl, CURLOPT_URL, ("ftp://" + Stored_IP + "/Users/" + Stored_Username + "/" +
		(FilenameFromFTP_Folder.has_filename() ?
			FilenameFromFTP_Folder.filename().string() : FilenameFromFTP_Folder.string())).c_str());
	//curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

	CURLcode res = curl_easy_perform(curl);
	if (CURLE_OK != res)
	{
#if __has_include("logger.h")
		Logger_Error_F("ReceiveFile() failed: {}\n", curl_easy_strerror(res));
#endif
		return false;
	}
	else
	{
		if (ftpfile.stream)
		{
			fclose(ftpfile.stream);
		}
	}
	return true;
}

bool network::ClientFTP::Connect(const std::string &ServerIP, const std::string &Login,
	const std::string &Password, const uint16_t &Port)
{
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0);
		curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (Login + ":" + Password).c_str());
		curl_easy_setopt(curl, CURLOPT_URL, ("ftp://" + ServerIP).c_str());
		curl_easy_setopt(curl, CURLOPT_PORT, Port > 0 ? Port : 2121);

		CURLcode ErrorCode = curl_easy_perform(curl);
		if (CURLE_OK != ErrorCode)
		{
#if __has_include("logger.h")
			Logger_Error_F("Connect() failed: {}\n", curl_easy_strerror(ErrorCode));
#endif
			Connected = false;
		}
		else
		{
			Stored_Username = Login;
			Stored_Password = Password;
			Stored_IP = ServerIP;
			Stored_Port = Port;
			Connected = true;
			return true;
		}
	}

	return false;
}

void network::ClientFTP::Disconnect()
{
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "QUIT");
	}
}

const std::string &network::ClientFTP::getIP()
{
	return Stored_IP;
}

