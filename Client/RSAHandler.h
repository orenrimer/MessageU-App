#pragma once
#include <string>
#include "rsa.h"
#include "osrng.h"
#include "Protocol.h"



static constexpr size_t BITS = 1024;


class RSAPublicWrapper {
public:
	static constexpr size_t KEY_SIZE = PUBLIC_KEY_SIZE;

	RSAPublicWrapper() {}
	virtual ~RSAPublicWrapper() = default;
	RSAPublicWrapper(const RSAPublicWrapper & other) = delete;
	RSAPublicWrapper(RSAPublicWrapper && other) noexcept = delete;
	RSAPublicWrapper& operator=(const RSAPublicWrapper & other) = delete;
	RSAPublicWrapper& operator=(RSAPublicWrapper && other) noexcept = delete;

	void loadPublicKey(const uint8_t* key);
	const std::string encrypteRSA(const uint8_t*, size_t);
private:
	CryptoPP::AutoSeededRandomPool m_rng;
	CryptoPP::RSA::PublicKey m_publicKey;
};




class RSAPrivateWrapper {
public:
	RSAPrivateWrapper() {}

	virtual ~RSAPrivateWrapper() = default;
	RSAPrivateWrapper(const RSAPrivateWrapper& other) = delete;
	RSAPrivateWrapper(RSAPrivateWrapper&& other) noexcept = delete;
	RSAPrivateWrapper& operator=(const RSAPrivateWrapper& other) = delete;
	RSAPrivateWrapper& operator=(RSAPrivateWrapper&& other) noexcept = delete;

	void randomizePrivateKey();
	void loadPrivateKey(const std::string&);
	const std::string getPrivateKey();
	const std::string getPublicKey() ;
	const std::string decrypteRSA(const uint8_t*, size_t);

private:
	CryptoPP::AutoSeededRandomPool m_rng;
	CryptoPP::RSA::PrivateKey m_privateKey;
};
