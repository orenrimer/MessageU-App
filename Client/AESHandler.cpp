#include "AESHandler.h"



void AESWrapper::generateKey() {
	const size_t length = sizeof(m_key);
	for (size_t i = 0; i < length; i += sizeof(size_t))
		_rdrand32_step(reinterpret_cast<size_t*>(&m_key[i]));
}



void AESWrapper::loadKey(const uint8_t* key, const size_t size) {
	if (size != SYM_KEY_SIZE) {
		throw std::length_error("key must be 16 bytes");
	}

	memcpy_s(m_key, SYM_KEY_SIZE, key, size);
}




const std::string AESWrapper::encrypt(const std::string& plainText) {
	return encrypt(reinterpret_cast<const uint8_t*>(plainText.c_str()), plainText.size());
}



const std::string AESWrapper::encrypt(const uint8_t* plain, size_t length) {
	CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE] = { 0 };	// for practical use iv should never be a fixed value!

	CryptoPP::AES::Encryption aesEncryption(m_key,SYM_KEY_SIZE);
	CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

	std::string cipher;
	CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
	stfEncryptor.Put(plain, length);
	stfEncryptor.MessageEnd();

	return cipher;
}


const std::string AESWrapper::decrypt(const uint8_t* cipher, size_t length)  {
	CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE] = { 0 };

	CryptoPP::AES::Decryption aesDecryption(m_key, SYM_KEY_SIZE);
	CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

	std::string decrypted;
	CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
	stfDecryptor.Put(cipher, length);
	stfDecryptor.MessageEnd();

	return decrypted;
}



void AESWrapper::getKey(uint8_t *buffer, const size_t size) {
	if (size != SYM_KEY_SIZE) {
		throw std::length_error("key must be 16 bytes");
	}

	memcpy_s(buffer, size, m_key, sizeof(m_key));
}