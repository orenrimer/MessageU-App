#include <modes.h>
#include <string>
#include <aes.h>
#include <filters.h>
#include <stdexcept>
#include <immintrin.h>	
#include "protocol.h"



class AESWrapper {
public:

	AESWrapper() : m_key{ 0 } {}
	virtual ~AESWrapper() = default;
	AESWrapper(const AESWrapper& other) = delete;
	AESWrapper(AESWrapper&& other) noexcept = delete;
	AESWrapper& operator=(const AESWrapper& other) = delete;
	AESWrapper& operator=(AESWrapper&& other) noexcept = delete;

	void generateKey();
	void loadKey(const uint8_t*, const size_t);
	const std::string encrypt(const std::string&);
	const std::string encrypt(const uint8_t*, size_t);
	const std::string decrypt(const uint8_t*, size_t);
	void getKey(uint8_t* buffer, const size_t size);
private:
	uint8_t m_key[SYM_KEY_SIZE];
};
