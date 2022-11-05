#pragma once
#include <iostream>
#include <format>
#include "SocketHandler.h"
#include "FileHandler.h"
#include "Protocol.h"
#include "ClientUI.h"
#include "RSAHandler.h"
#include "AESHandler.h"
#include "Utils.h"



constexpr auto CLIENT_FILE_PATH = "me.info";


class ClientHandler {
public:
	struct Client {
		ClientID clientId;
		std::string name;
		uint8_t publicKey[PUBLIC_KEY_SIZE];
		uint8_t symKey[SYM_KEY_SIZE];
		bool m_isRegistered;

		Client() : publicKey { 0 }, symKey{ 0 }, m_isRegistered(false) {}
	};

	ClientHandler();
	virtual ~ClientHandler();
	ClientHandler(const ClientHandler& other) = delete;
	ClientHandler(ClientHandler&& other) noexcept = delete;
	ClientHandler& operator=(const ClientHandler& other) = delete;
	ClientHandler& operator=(ClientHandler&& other) noexcept = delete;

	bool clientMain();
	bool isRegistered() { return m_this.m_isRegistered; };

private:
	bool handleRegistrationRequest();
	bool handleGetPublicKeyRequest(Client* client=NULL, bool display=false);
	bool handleClientsListRequest(bool=false);
	bool handleGetUnreadMessages();
	bool handleSendMsgRequest(MessageType);
	bool handelUnkownPayloadRequest(RequestCode, ResponseCode, uint8_t*&, uint32_t&);
	bool setClientInfo();
	bool getClientInfo();
	bool getClient(const std::string&, ClientID*, Client&, int);
	bool isValidResponse(const ResponseHeader&, ResponseCode);
	bool isValidUsername(const std::string&);

	Client m_this;
	std::vector<Client> m_clients;
	ClientUI* m_ui;
	RSAPrivateWrapper* m_rsaDecryptor;
	SocketHandler* m_socketHandler;
	FileHandler* m_fileHandler;
};