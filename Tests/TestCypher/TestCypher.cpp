#include <iostream>
#include "sha.h"
#include "cryptlib.h"
#include "pch.h"
#include "gzip.h"

std::string GzipEncrypt(const std::string &text)
{
	std::string compressed;

	CryptoPP::StringSource ss(text, true,
		new CryptoPP::Gzip(
			new CryptoPP::StringSink(compressed)
	));
	return String2HEX(compressed);
}
std::string GzipDecrypt(const std::string &text)
{
	std::string uncompressed;

	CryptoPP::StringSource ss(HEX2String(text), true,
		new CryptoPP::Gunzip(
			new CryptoPP::StringSink(uncompressed)
	));
	return uncompressed;
}

#include "nlohmann/json.hpp"

int main()
{
	nlohmann::json BuildCookie;
	BuildCookie["Cookie"] = { { "Unique_ID", "ID228" }, {"Name", "Login" }, { "Domain", "*" },
	{ "Value", "127.0.0.1" } };
	std::cout << BuildCookie.dump() << std::endl;

	auto Crypted = nlohmann::json::to_ubjson(BuildCookie);

	std::string msg = reinterpret_cast<char *>(Crypted.data());
	msg.resize(Crypted.size());

	std::cout << (msg = GzipEncrypt(msg)) << std::endl;
	std::cout << (msg = GzipDecrypt(msg)) << std::endl;
	std::cout << (msg = nlohmann::json::from_ubjson(msg).dump()) << std::endl;
}
