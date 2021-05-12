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
	HexEncoder encoder(new FileSink(std::cout));
	std::string digest;
	Weak::MD5 hash;

	hash.Update((const byte*)&data[0], data.size());
	digest.resize(hash.DigestSize());
	hash.Final((byte*)&digest[0]);

	StringSource(digest, true, new Redirector(encoder));

	std::transform(digest.begin(), digest.end(), digest.begin(), ::toupper);
	return digest;
}
