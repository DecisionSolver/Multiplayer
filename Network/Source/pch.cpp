#include "pch.h"

const std::string md5_from_file(const std::string &path)
{
	using namespace CryptoPP;
	Weak1::MD5 md;
	const size_t size = Weak1::MD5::DIGESTSIZE * 2;
	byte buf[size] = { 0 };
	FileSource(
		path.c_str(), true,
		new HashFilter(
			md, new HexEncoder(new ArraySink(buf, size))));
	std::string strHash = std::string(reinterpret_cast<const char*>(buf), size);

	std::transform(strHash.begin(), strHash.end(), strHash.begin(), ::toupper);
	return strHash;
}
const std::string md5_from_buffer(const std::string &data)
{
	using namespace CryptoPP;
	byte digest[Weak::MD5::DIGESTSIZE];

	Weak::MD5 hash;
	hash.CalculateDigest(digest, (const byte*)data.c_str(), data.length());

	HexEncoder encoder;
	std::string output;

	encoder.Attach(new StringSink(output));
	encoder.Put(digest, sizeof(digest));
	encoder.MessageEnd();

	std::transform(output.begin(), output.end(), output.begin(), ::toupper);
	
	return output;
}
