#pragma once
#include "pch.h"
#include <curl/curl.h>

class HTTP
{
public:
	HTTP() { curl = curl_easy_init(); }
	~HTTP() { curl_easy_cleanup(curl); }
	std::string GET(const std::string &URL);
private:
	CURL *curl = nullptr;
	CURLcode res = (CURLcode)0;
	
	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
};