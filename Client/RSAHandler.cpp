#include "RSAHandler.h"



void RSAPublicWrapper::loadPublicKey(const uint8_t* key) {
	CryptoPP::StringSource ss((key), sizeof(key), true);
	m_publicKey.Load(ss);
}


void RSAPrivateWrapper::randomizePrivateKey() {
	m_privateKey.Initialize(m_rng, BITS);
}


void RSAPrivateWrapper::loadPrivateKey(const std::string& key) {
	CryptoPP::StringSource ss(key, true);
	m_privateKey.Load(ss);
}


const std::string RSAPublicWrapper::encrypteRSA(const uint8_t* plainText, size_t size) {
	std::string cipherText;
	CryptoPP::RSAES_OAEP_SHA_Encryptor e(m_publicKey);
	CryptoPP::StringSource ss(plainText, size, true, new CryptoPP::PK_EncryptorFilter(m_rng, e, new CryptoPP::StringSink(cipherText)));
	return cipherText;
}


const std::string RSAPrivateWrapper::decrypteRSA(const uint8_t* cipherText, size_t size) {
	std::string out;
	CryptoPP::RSAES_OAEP_SHA_Decryptor d(m_privateKey);
	CryptoPP::StringSource ss(cipherText, size, true, new CryptoPP::PK_DecryptorFilter(m_rng, d, new CryptoPP::StringSink(out)));
	return out;
}


const std::string RSAPrivateWrapper::getPublicKey() {
	std::string key;
	const CryptoPP::RSAFunction publicKey(m_privateKey);
	CryptoPP::StringSink ss(key);
	publicKey.Save(ss);
	return key;
}


const std::string RSAPrivateWrapper::getPrivateKey() {
	std::string key;
	CryptoPP::StringSink ss(key);
	m_privateKey.Save(ss);
	return key;
}
