#include "Utils.h"



std::string Utils::encodeBase64(const std::string& str) {
	std::string encoded;
	CryptoPP::StringSource ss(str, true, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(encoded)));
	return encoded;
}


std::string Utils::decodeBase64(const std::string& str) {
	std::string decoded;
	CryptoPP::StringSource ss(str, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));
	return decoded;
}


/**
 * Try to convert bytes to hex string representation.
 * Return empty string upon failure.
 */
std::string Utils::bytesToHex(const uint8_t* buffer, const size_t size) {
	if (size == 0 || buffer == nullptr) return "";

	const std::string byteString(buffer, buffer + size);

	try {
		return boost::algorithm::hex(byteString);
	} catch (...) {
		return "";
	}
}


/**
 * Try to convert hex string to bytes string.
 * Return empty string upon failure.
 */
std::string Utils::hexToBytes(const std::string& hexString) {
	if (hexString.empty()) return "";

	try {
		std::vector<uint8_t> bytes;
		return boost::algorithm::unhex(hexString);
	} catch (...) {
		return "";
	}
}


/**
 * Return current timestamp as sting.
 
std::string Utils::getTimestamp()
{
	const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	return std::to_string(now.count());
}

*/