#include "Client.h"


ClientHandler::ClientHandler() : m_ui(nullptr), m_fileHandler(nullptr), m_rsaDecryptor(nullptr), m_socketHandler(nullptr) {
	m_ui = new ClientUI;
	m_fileHandler = new FileHandler;
	m_socketHandler = new SocketHandler;
	m_rsaDecryptor = new RSAPrivateWrapper;

	// if the user is registers, load all of his info
	if (m_fileHandler->fileExists(CLIENT_FILE_PATH)) {
		if (!getClientInfo()) exit(1);
		m_this.m_isRegistered = true;
	}
}


ClientHandler::~ClientHandler() {
	delete m_ui;
	delete m_fileHandler;
	delete m_socketHandler;
	delete m_rsaDecryptor;
}



bool ClientHandler::clientMain() {
	bool success = true;

	while (success) {
		ClientUI::MenuOption opt = m_ui->display();

		if (opt != ClientUI::MenuOption::REGISTER && opt != ClientUI::MenuOption::NONE_OPTION && opt != ClientUI::MenuOption::EXIT && !isRegistered()) {
			std::cout << "You must register to preform this action, please choose 'REGISTER' or press '0' to exit." << std::endl;
			continue;
		}

		switch (opt) {
		case ClientUI::MenuOption::REGISTER:
			success = handleRegistrationRequest();
			break;
		case ClientUI::MenuOption::GET_CLIENT_LIST:
			success = handleClientsListRequest(true);
			break;
		case ClientUI::MenuOption::GET_PUBLIC_KEY:
			success = handleGetPublicKeyRequest(NULL, true);
			break;
		case ClientUI::MenuOption::GET_UNREAD_MSG:
			success = handleGetUnreadMessages();
			break;
		case ClientUI::MenuOption::SEND_MSG:
			success = handleSendMsgRequest(MessageType::TEXT_MESSAGE);
			break;
		case ClientUI::MenuOption::REQ_SYM_KEY:
			success = handleSendMsgRequest(MessageType::REQUEST_SYM_KEY);
			break;
		case ClientUI::MenuOption::SEND_SYM_KEY:
			success = handleSendMsgRequest(MessageType::SEND_SYM_KEY);
			break;
		case ClientUI::MenuOption::SEND_FILE:
			success = handleSendMsgRequest(MessageType::FILE_MSG);
			break;
		case ClientUI::MenuOption::EXIT:
			std::cout << "Thank you, hope to see you soon!" << std::endl;
			return success;
		default:
			std::cout << "Invalid option, please choose again or press '0' to exit." << std::endl;
			break;
		}
	}
	return success;
}



bool ClientHandler::handleRegistrationRequest() {
	if (m_this.m_isRegistered) {
		std::cout << "You are already registered!" << std::endl;
		return true;
	}

	std::string userName;
	do {
		userName = m_ui->getCleanInput("Please enter a user name: ");
	} while (!isValidUsername(userName));
	
	RegistrationRequest req;
	RegistrationResponse resp;

	memcpy(req.name, userName.c_str(), NAME_SIZE);

	m_rsaDecryptor->randomizePrivateKey();
	const std::string publicKey = m_rsaDecryptor->getPublicKey();

	if (publicKey.size() != PUBLIC_KEY_SIZE) {
		std::cout << "Invalid public key length!" << std::endl;
		return false;
	}

	memcpy(req.publicKey, publicKey.c_str(), PUBLIC_KEY_SIZE);

	if (!m_socketHandler->socketWrapper(reinterpret_cast<const uint8_t*>(&req), sizeof(req), reinterpret_cast<uint8_t*>(&resp), sizeof(resp))) {
		std::cout << "Failed to process REGISTRATION REQUEST" << std::endl;
		return false;
	}

	if (!isValidResponse(resp.header, ResponseCode::REGISTER_SUCCESS)) {
		std::cout << "Invalid response, can not complete registration" << std::endl;
		return false;
	}

	m_this.name = userName;
	m_this.clientId = resp.clientId;
	memcpy(m_this.publicKey, publicKey.c_str(), PUBLIC_KEY_SIZE);
	
	if (!setClientInfo()) {
		std::cout << "Error while trying to save your details" << std::endl;
		return false;
	}

	m_this.m_isRegistered = true;
	return true;
}


bool ClientHandler::handleClientsListRequest(bool show) {
	ClientsListResponse resp;

	if (!handelUnkownPayloadRequest(RequestCode::GET_CLIENTS_LIST, ResponseCode::GET_CLIENTS_LIST_SUCCESS, resp.payload, resp.header.payloadtSize)) {
		return false;
	}

	const size_t payloadSize = resp.header.payloadtSize;
	const size_t clientBlockSize = CLIENT_ID_SIZE + NAME_SIZE;	// 271 bytes
	size_t bytesRead = 0;

	UnpackClient temp;
	Client newClient;

	m_clients.clear();
	while (bytesRead < payloadSize) {
		memcpy(&temp, &resp.payload[bytesRead], clientBlockSize);
		newClient.clientId = temp.clientId;
		temp.name[sizeof(temp.name) - 1] = '\0';
		newClient.name = reinterpret_cast<char*>(temp.name);
		m_clients.push_back(newClient);
		bytesRead += clientBlockSize;
	}
	
	if (show) {
		std::cout << "Clients:" << std::endl;
		for (const auto& client : m_clients) {
			std::cout << "\n\tname: " << client.name << std::endl;
			std::cout << "\tID: " << client.clientId.id << std::endl;
		}
	}

	delete[] resp.payload;
	return true;
}


bool ClientHandler::handleGetUnreadMessages() {
	UnreadMessagesResponse resp;
	 
	if (!handelUnkownPayloadRequest(RequestCode::GET_UNREAD_MESSAGES, ResponseCode::GET_UNREAD_MESSAGES_SUCCESS, resp.payload, resp.header.payloadtSize)) {
		return false;
	}
	
	// if the user has no messages
	if (resp.header.payloadtSize == 0) {
		std::cout << "No new messages..." << std::endl;
		return true;
	}

	const size_t payloadSize = resp.header.payloadtSize;
	const size_t msgHeaderSize = sizeof(UnpackMessage);
	size_t msgSize = 0;


	if (payloadSize < msgHeaderSize) return false;

	UnpackMessage msgHeader;
	uint32_t bytesRead = 0;
	uint8_t* p = resp.payload;
	uint8_t* msg;
	AESWrapper aes;
	Client from;
	while (bytesRead < payloadSize) {
		memcpy(&msgHeader, p, msgHeaderSize);
		p += msgHeaderSize;
		msgSize = msgHeader.msgSize;
	
		if (msgSize == 0) {
			bytesRead = bytesRead + msgHeaderSize;
			continue;
		} else if (bytesRead + msgSize > payloadSize) {
			return false;
		}


		if (!getClient("", &msgHeader.clientId, from, 2)) {
			std::cout << "FROM: Unknown {ID: " << msgHeader.clientId.id << "}" << std::endl;
		} else {
			std::cout << "FROM: " << from.name << std::endl;
		}
		

		if (strlen(reinterpret_cast<char*>(from.symKey)) > 0) {
			aes.loadKey(from.symKey, sizeof(from.symKey));
			std::string data;
			try {
				data = aes.decrypt(p, msgSize);
				std::cout << "\t" << data << std::endl;
			} catch (...) {
				std::cout << "\tCan not decrypt message content... " << std::endl;
			}
		} else {
			std::cout << "\tCan not get client symmetric key" << std::endl;
		}

		p += msgSize;
		bytesRead = bytesRead + msgHeaderSize + msgSize;
	}

	delete[] resp.payload;
	return true;
}


bool ClientHandler::handleGetPublicKeyRequest(Client* client, bool display) {
	std::string userName;

	if (client == nullptr) {
		do {
			userName = m_ui->getCleanInput("Please enter the name of the user you want to get the key for: ");
		} while (!isValidUsername(userName));
	} else {
		userName = client->name;
	}

	PublicKeyRequest req;
	PublicKeyResponse resp;

	req.header.clientId = m_this.clientId;
	memcpy_s(req.name, NAME_SIZE, userName.c_str(), userName.length());

	if (!m_socketHandler->socketWrapper(reinterpret_cast<const uint8_t*>(&req), sizeof(req), reinterpret_cast<uint8_t*>(&resp), sizeof(resp))) {
		std::cout << "Failed to process GET PUBLIC KEY Request" << std::endl;
		return false;
	}

	if (!isValidResponse(resp.header, ResponseCode::GET_PUBLIC_KEY_SUCCESS)) {
		std::cout << "Invalid response, can not complete action" << std::endl;
		return false;
	}

	// store the public key
	memcpy_s(client->publicKey, sizeof(client->publicKey), resp.publicKey, sizeof(resp.publicKey));
	if(display) std::cout << "User info:\n\tname: " << req.name << "\n\tid: " << resp.clientID.id << "\n\tpublic key: " << resp.publicKey << std::endl;
	return true;
}



bool ClientHandler::handleSendMsgRequest(MessageType msgType) {
	std::string userName = m_ui->getCleanInput("Please enter the recipient's user name: ");

	handleClientsListRequest();

	Client recipient;
	if (!getClient(userName, NULL, recipient, 1)) {
		std::cout << "Invalid user name, no user by this name." << std::endl;
		return true;
	} else if (m_this.clientId == recipient.clientId) {
		std::cout << "You can not send messages to yourself" << std::endl;
		return true;
	}

	SendMessageRequest req;
	req.header.clientId = m_this.clientId;
	req.clientId = recipient.clientId;

	// encode message
	if (!recipient.symKey) {
		std::cout << "Couldn't find " << recipient.name << "'s symmetric key.";
		return false;
	}

	std::string msg;
	std::string encrypted;
	AESWrapper aes;
	RSAPublicWrapper rsa;

	switch (msgType) {
		case REQUEST_SYM_KEY:
			req.msgType = MessageType::REQUEST_SYM_KEY;
			msg = "Request for symmetric key";

			req.contentSize = msg.length();
			req.msgContent = new uint8_t[req.contentSize];
			memcpy(req.msgContent, msg.c_str(), req.contentSize);
			break;
		case SEND_SYM_KEY:
			req.msgType = MessageType::SEND_SYM_KEY;
			aes.generateKey();	// generate symmetric key
			aes.getKey(recipient.symKey, sizeof(recipient.symKey));

			if (recipient.publicKey[0] == '\0') {
				if (!handleGetPublicKeyRequest(&recipient)) {
					std::cout << "Failed to get recipient's public key" << std::endl;
					return false;
				}
			}

			break;
		case TEXT_MESSAGE:
		case FILE_MSG:
			msg = m_ui->getCleanInput("Please enter the message: ");
			if (msg.empty()) {
				std::cout << "You must type a message" << std::endl;
				return true;
			}

			req.msgType = (msgType == MessageType::TEXT_MESSAGE) ? MessageType::TEXT_MESSAGE : MessageType::FILE_MSG;
			aes.loadKey(recipient.symKey, sizeof(recipient.symKey));
			encrypted = aes.encrypt(msg);
			req.contentSize = encrypted.size();
			req.header.payloadSize = req.payloadSizeWithoutMsg() + req.contentSize;

			// copy the encoded message into the payload
			req.msgContent = new uint8_t[req.contentSize];
			memcpy(req.msgContent, encrypted.c_str(), req.contentSize);
			break;
		default:
			std::cout << "Invalid message type, can not send message" << std::endl;
			return true;
	}


	MessageSentResponse resp;

	if (!m_socketHandler->socketWrapper(reinterpret_cast<const uint8_t *>(&req), sizeof(req), reinterpret_cast<uint8_t*>(&resp), sizeof(resp))) {
		std::cout << "Error while trying to connect with server." << std::endl;
		delete[] req.msgContent;
		return false;
	}

	if (!isValidResponse(resp.header, ResponseCode::MESSAGE_SENT_SUCCESS)) {
		std::cout << "Invalid response, can not complete action" << std::endl;
		delete[] req.msgContent;
		return false;
	}

	std::cout << resp.clientId.id << std::endl;
	std::cout << resp.msgID << std::endl;

	delete[] req.msgContent;
	return true;
}




bool ClientHandler::handelUnkownPayloadRequest(RequestCode reqCode, ResponseCode respCode, uint8_t*& payload, uint32_t& payloadSize){
	RequestHeader req(reqCode);
	ResponseHeader respHeader;

	req.clientId = m_this.clientId;
	uint8_t buffer[PACKET_SIZE];

	if (!m_socketHandler->socketWrapper(reinterpret_cast<const uint8_t*>(&req), sizeof(req), buffer, PACKET_SIZE, false)) {
		std::cout << "Failed to send GET CLIENT LIST Request" << std::endl;
		return false;
	}

	memcpy(&respHeader, buffer, sizeof(ResponseHeader));

	if (!isValidResponse(respHeader, respCode)) {
		std::cout << "Invalid response, can not complete action" << std::endl;
		return false;
	}

	payloadSize = respHeader.payloadtSize;
	if (payloadSize == 0) return true;

	payload = new uint8_t[payloadSize];
	uint8_t* ptr = buffer + sizeof(ResponseHeader);
	size_t toRead = std::min(sizeof(buffer) - sizeof(ResponseHeader), payloadSize);
	memcpy(payload, ptr, toRead);

	size_t totalRead = toRead;
	ptr = payload + totalRead;

	while (totalRead < payloadSize) {
		toRead = std::min(PACKET_SIZE, payloadSize - totalRead);
		if (!m_socketHandler->read(buffer, toRead)) {
			delete[] payload;
			payload = nullptr;
			payloadSize = 0;
			std::cout << "Failed to read GET CLIENT LIST payload" << std::endl;
			m_socketHandler->closeSocket();
			return false;
		}

		memcpy(ptr, buffer, toRead);
		totalRead += toRead;
		ptr += toRead;
	}

	m_socketHandler->closeSocket();
	return true;
}


bool ClientHandler::isValidResponse(const ResponseHeader& header, ResponseCode expectedcode) {
	if (header.code != expectedcode) return false;

	if (sizeof(header) != sizeof(ResponseHeader)) return false;

	size_t expectedPayloadSize = 0;
	if (header.code == ResponseCode::REGISTER_SUCCESS) {
		expectedPayloadSize = sizeof(RegistrationResponse) - sizeof(header);
	} else if (header.code == ResponseCode::GET_PUBLIC_KEY_SUCCESS) {
		expectedPayloadSize = sizeof(PublicKeyResponse) - sizeof(header);
	}else if (header.code == ResponseCode::MESSAGE_SENT_SUCCESS) {
		expectedPayloadSize = sizeof(MessageSentResponse) - sizeof(header);
	} else {
		return true;
	}

	return (header.payloadtSize == expectedPayloadSize);
}


bool ClientHandler::isValidUsername(const std::string& userName) {
	if (userName.length() == 0 || userName.length() >= NAME_SIZE) {
		std::cout << "Invalid user name, can not be longer than " << NAME_SIZE << " characters." << std::endl;
		return false;
	}

	for (auto& ch : userName) {
		if (!std::isalnum(ch) && !std::isspace(ch) && ch != '\t') {
			std::cout << "Invalid user name, can only contain alpha numerical characters." << std::endl;
			return false;
		}
	}

	return true;
}


bool ClientHandler::setClientInfo() {
	if (!m_fileHandler->write(CLIENT_FILE_PATH, m_this.name)) return false;

	const std::string hexID = Utils::bytesToHex(m_this.clientId.id, sizeof(m_this.clientId.id));
	if (hexID == "" || !m_fileHandler->write(CLIENT_FILE_PATH, hexID)) return false;

	const std::string encodedPrivateKey = Utils::encodeBase64(m_rsaDecryptor->getPrivateKey());
	if (encodedPrivateKey == "" || !m_fileHandler->write(CLIENT_FILE_PATH, encodedPrivateKey)) return false;

	return true;
}


bool ClientHandler::getClientInfo() {
	std::string line;
	if (!m_fileHandler->readLine(CLIENT_FILE_PATH, line)) {
		std::cout << "Error while trying to read user name from file" << std::endl;
		return false;
	} else if (!isValidUsername(line)) {
		return false;
	}

	m_this.name = line;

	if (!m_fileHandler->readLine(CLIENT_FILE_PATH, line)) {
		std::cout << "Error while trying to read client ID name from file" << std::endl;
		return false;
	}

	std::string decryptedID = Utils::hexToBytes(line);
	if (decryptedID == "" || decryptedID.size() != CLIENT_ID_SIZE) return false;
	memcpy(m_this.clientId.id, decryptedID.c_str(), CLIENT_ID_SIZE);
	
	std::string key;
	while (m_fileHandler->readLine(CLIENT_FILE_PATH, line)) {
		key.append(Utils::decodeBase64(line));
	}
	m_fileHandler->closeFS();

	if (key.empty()) {
		std::cout << "Error while trying to read private key from file" << std::endl;
		return false;
	}

	try {
		m_rsaDecryptor->loadPrivateKey(key);
	} catch (...) {
		return false;
	}
	
	return true;
}

bool ClientHandler::getClient(const std::string& name, ClientID* id, Client& outClient, int selector) {
	if (m_clients.empty()) return false;

	for (const auto& client : m_clients) {
		if (selector == 1) {
			if (name == client.name) {
				outClient = client;
				return true;
			}
		} else if (selector == 2) {
			if (id == nullptr) return false;
			if (*id == client.clientId) {
				outClient = client;
				return true;
			}
		} else {
			return false;
		}
	}
	return false;
}
