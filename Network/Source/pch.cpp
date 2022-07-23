#include "pch.h"
#include "crypto++/include/sha.h"
#include <md5.h>

const std::string md5_from_file(const std::string &path)
{
	CryptoPP::Weak1::MD5 md;
	const size_t size = CryptoPP::Weak1::MD5::DIGESTSIZE * 2;
	CryptoPP::byte buf[size] = { 0 };
	CryptoPP::FileSource(
		path.c_str(), true,
		new CryptoPP::HashFilter(
			md, new CryptoPP::HexEncoder(new CryptoPP::ArraySink(buf, size))));
	std::string strHash = std::string(reinterpret_cast<const char*>(buf), size);

	std::transform(strHash.begin(), strHash.end(), strHash.begin(), ::toupper);
	return strHash;
}
const std::string md5_from_buffer(const std::string &data)
{
	CryptoPP::byte digest[CryptoPP::Weak::MD5::DIGESTSIZE];

	CryptoPP::Weak::MD5 hash;
	hash.CalculateDigest(digest, (const CryptoPP::byte *)data.c_str(), data.length());

	CryptoPP::HexEncoder encoder;
	std::string output;

	encoder.Attach(new CryptoPP::StringSink(output));
	encoder.Put(digest, sizeof(digest));
	encoder.MessageEnd();

	std::transform(output.begin(), output.end(), output.begin(), ::toupper);

	return output;
}

std::string String2HEX(const std::string &input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();

	std::string output;
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const unsigned char c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}
std::string HEX2String(const std::string &input)
{
	std::string destination;
	CryptoPP::StringSource ss(input, true, new CryptoPP::HexDecoder(new CryptoPP::StringSink(destination)));
	return destination;
}

std::string SHA256(std::string data)
{
	CryptoPP::byte const* pbData = (CryptoPP::byte *)data.data();
	unsigned int nDataLen = data.length();
	CryptoPP::byte abDigest[CryptoPP::SHA256::DIGESTSIZE];

	CryptoPP::SHA256().CalculateDigest(abDigest, pbData, nDataLen);

	return std::string((char*)abDigest, CryptoPP::SHA256::DIGESTSIZE);
}

std::string Crypt(const std::string &data)
{
	return md5_from_buffer(String2HEX(SHA256(data)));
}

void recursive_iterate(const nlohmann::json &j, std::function<bool(nlohmann::json::const_iterator)> f)
{
	for (auto it = j.begin(); it != j.end(); ++it)
	{
		if (it->is_structured())
			recursive_iterate(*it, f);
		else
		{
			if (f(it))
				return;
		}
	}
}