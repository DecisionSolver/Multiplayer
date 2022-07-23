#include "HTTP.h"

size_t HTTP::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

std::string HTTP::GET(const std::string &URL)
{
	std::string readBuffer;
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTP::WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
	}

	return readBuffer;
}
