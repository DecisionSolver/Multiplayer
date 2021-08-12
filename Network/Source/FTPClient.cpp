#include "FTPClient.h"

/*
#if defined(_DEBUG)
	#define IP "ftp://192.168.121.1"
#else
	#define IP "ftp://188.210.240.246"
#endif
*/

size_t FTPClient::getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int r = 0;
	long len = 0;

	r = sscanf((LPCSTR)ptr, "Content-Length: %ld\n", &len);
	if (r)
		*((long *)stream) = len;

	return size * nmemb;
}

size_t FTPClient::write_callback(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out = (FtpFile *)stream;
	if (!out->stream && !out->filename.empty()) {
		/* open file for writing */
		out->stream = fopen(out->filename.c_str(), "wb");
		if (!out->stream)
			return 1; /* failure, can't open file to write */
	}
	if (out->stream)
		return fwrite(buffer, size, nmemb, out->stream);
	else
		return size * nmemb;
}
size_t FTPClient::read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	FILE *ThisFile = (FILE *)stream;
	if (ferror(ThisFile))
		return CURL_READFUNC_ABORT;

	return fread(ptr, size, nmemb, ThisFile) * size;
}

FTPClient::FTPClient()
{
	// Init Winsock
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	/* Check for errors */
	if (res != CURLE_OK)
	{
#if __has_include("logger.h")
		Logger_Error_F("curl_global_init() failed: %s\n", curl_easy_strerror(res));
#endif
	}
	// Create And Init CURL
	curl = curl_easy_init();

	// Bind Upload And Download Function
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

#if defined(USE_SSL)
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER , 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST , 1);
	curl_easy_setopt(curl, CURLOPT_CAINFO , "./ca.cert");
#endif

	// Enable Upload
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);

#if defined(_DEBUG)
	// Enable Full-log Message
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

}
FTPClient::~FTPClient()
{
	if (curl)
		curl_easy_cleanup(curl);
	curl_global_cleanup();
}

bool FTPClient::SendFile(const boost::filesystem::path &FilePath)
{
	if (!Connected)
	{
		Logger_Warn("You're not connected to the FTP Server! Aborting!");
		return false;
	}
	FILE *File;
	long uploaded_len = 0; 
	CURLcode res = CURLE_GOT_NOTHING;
	int c = 0;
	
	File = fopen(FilePath.string().c_str(), "rb");
	if (!File)
		return false;
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, ("ftp://" + IP + "/Users/" + UserName + "/" + FilePath.filename().string()).c_str());
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getcontentlengthfunc);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &uploaded_len);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)uploaded_len);

	curl_easy_setopt(curl, CURLOPT_READDATA, File);
	for (c = 0; (res != CURLE_OK) && (c < 1); c++)
	{
		if (c)
		{
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
			curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK)
				continue;
			curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

			fseek(File, uploaded_len, SEEK_SET);

			curl_easy_setopt(curl, CURLOPT_APPEND, 1L);
		}
		else
			curl_easy_setopt(curl, CURLOPT_APPEND, 0L);

		res = curl_easy_perform(curl);
	}
	
	fclose(File); /* close the local file */
	
	if (res != CURLE_OK && res != CURLE_PARTIAL_FILE)
	{
#if __has_include("logger.h")
		Logger_Error_F("SendFile() failed: %s\n", curl_easy_strerror(res));
#endif
		return false;
	}
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
	return true;
}

bool FTPClient::ReceiveFile(const boost::filesystem::path &Path, const boost::filesystem::path &Where)
{
	if (!Connected)
	{
		Logger_Warn("You're not connected to the FTP Server! Aborting!");
		return false;
	}
	FtpFile ftpfile =
	{
	   "",
	   nullptr
	};
	ftpfile.filename = Where.string();

	curl_easy_setopt(curl, CURLOPT_URL, ("ftp://" + IP + "/Users/" + Path.string()).c_str());
	//curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

	CURLcode res = curl_easy_perform(curl);
	if (CURLE_OK != res)
	{
#if __has_include("logger.h")
		Logger_Error_F("ReceiveFile() failed: %s\n", curl_easy_strerror(res));
#endif
		return false;
	}
	else
	{
		if (ftpfile.stream)
			fclose(ftpfile.stream);
	}
	return true;
}

bool FTPClient::Connect(const std::string &ServerIP, const std::string &Login,
	const std::string &Pass, const uint16_t &Port)
{
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0);
		curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (Login + ":" + Pass).c_str());
		curl_easy_setopt(curl, CURLOPT_URL, ("ftp://" + ServerIP).c_str());
		curl_easy_setopt(curl, CURLOPT_PORT, Port > 0 ? Port : port_);

		CURLcode res = curl_easy_perform(curl);
		if (CURLE_OK != res)
		{
#if __has_include("logger.h")
			Logger_Error_F("Connect() failed: %s\n", curl_easy_strerror(res));
#endif
			const_cast<bool &>(Connected) = false;

			/* we failed */
			return false;
		}
		else
		{
			const_cast<std::string &>(UserName) = Login;
			const_cast<std::string &>(IP) = ServerIP;
			const_cast<uint16_t &>(port_) = Port;
			const_cast<bool &>(Connected) = true;
			return true;
		}
	}

	return false;
}

void FTPClient::Disconnect()
{
	if (curl)
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "QUIT");
}

const uint16_t &FTPClient::getPort()
{
	return port_;
}
const std::string &FTPClient::getIP()
{
	return IP;
}

