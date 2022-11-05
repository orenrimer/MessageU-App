#pragma once
#include <string>
#include <base64.h>
#include <boost/algorithm/hex.hpp>
#include <chrono>


class Utils {

public:
	static std::string encodeBase64(const std::string& str);
	static std::string decodeBase64(const std::string& str);
	static std::string bytesToHex(const uint8_t* buffer, const size_t size);
	static std::string hexToBytes(const std::string& hexString);
};

