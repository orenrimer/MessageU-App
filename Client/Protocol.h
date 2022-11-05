#pragma once
#include <cstdint>
#include <string>


constexpr int CLIENT_VERSION = 1;
constexpr size_t CLIENT_ID_SIZE = 16;
constexpr size_t NAME_SIZE = 255;
constexpr size_t PUBLIC_KEY_SIZE = 160;
constexpr size_t MESSAGE_ID_SIZE = 4;
constexpr size_t SYM_KEY_SIZE = 16;  


enum  RequestCode {
    REGISTER_CLIENT = 1000,
    GET_CLIENTS_LIST = 1001,
    GET_PUBLIC_KEY = 1002,
    SEND_MESSAGE = 1003,
    GET_UNREAD_MESSAGES = 1004
};
 

enum ResponseCode {
    REGISTER_SUCCESS = 2000,
    GET_CLIENTS_LIST_SUCCESS = 2001,
    GET_PUBLIC_KEY_SUCCESS = 2002,
    MESSAGE_SENT_SUCCESS = 2003,
    GET_UNREAD_MESSAGES_SUCCESS = 2004,
    GENERIC_ERROR = 9000
}; 


enum MessageType {
    REQUEST_SYM_KEY = 1,
    SEND_SYM_KEY = 2,
    TEXT_MESSAGE = 3,
    FILE_MSG = 4,
    NONE_MESSAGE = 0
};



#pragma pack(push, 1)
    struct ClientID {
        uint8_t id[CLIENT_ID_SIZE];
        ClientID() : id{ 0 } {}

        bool operator==(const std::string& other) const {
            for (size_t i = 0; i < CLIENT_ID_SIZE; ++i)
                if (id[i] != other[i])
                    return false;
            return true;
        }

        bool operator==(const ClientID& otherID) const {
            for (size_t i = 0; i < CLIENT_ID_SIZE; ++i)
                if (id[i] != otherID.id[i])
                    return false;
            return true;
        }

        bool operator!=(const ClientID& otherID) const {
            return !(*this == otherID);
        }

    };


	struct RequestHeader {
        ClientID clientId;
		uint8_t version;
		uint16_t code;
		uint32_t payloadSize;

        RequestHeader(const uint16_t code) : version(CLIENT_VERSION), code(code), payloadSize(0) {}
        RequestHeader(const uint16_t code, const uint32_t paySize) : version(CLIENT_VERSION), code(code), payloadSize(paySize) {}
	};



    struct RegistrationRequest {
        RequestHeader header;
        uint8_t name[NAME_SIZE];
        uint8_t publicKey[PUBLIC_KEY_SIZE];

        RegistrationRequest() : header(REGISTER_CLIENT, getPayloadSize()) , name{ 0 }, publicKey{ 0 } {}
        uint32_t getPayloadSize() { return sizeof(name) + sizeof(publicKey); }
    };



    struct PublicKeyRequest {
        RequestHeader header;
        uint8_t name[NAME_SIZE];

        PublicKeyRequest() : header(GET_PUBLIC_KEY, NAME_SIZE), name { 0 } {}
    };


    struct SendMessageRequest {
        RequestHeader header;
        ClientID clientId;
        uint8_t msgType;
        uint32_t contentSize;
        uint8_t* msgContent;
         
        SendMessageRequest() : header(SEND_MESSAGE), msgType(NONE_MESSAGE), contentSize(0), msgContent(nullptr) {}
        uint32_t payloadSizeWithoutMsg() {return sizeof(clientId) + sizeof(contentSize) + sizeof(msgType); }
    };


    struct ResponseHeader {
        uint8_t version;
        uint16_t code;
        uint32_t payloadtSize;

        ResponseHeader() : version(0), code(0), payloadtSize(0) {}
    };


    struct RegistrationResponse {
        ResponseHeader header;
        ClientID clientId;
    };


    struct ClientsListResponse {
        ResponseHeader header;
        uint8_t* payload;

        ClientsListResponse() : payload(nullptr) {}
    };


    struct PublicKeyResponse {
        ResponseHeader header;
        ClientID clientID;
        uint8_t publicKey[PUBLIC_KEY_SIZE];

        PublicKeyResponse() : publicKey{ 0 } {}
    };


    struct MessageSentResponse {
        ResponseHeader header;
        ClientID clientId;
        uint32_t msgID;

        MessageSentResponse() : msgID(0){}
    };


    struct UnreadMessagesResponse {
        ResponseHeader header;
        uint8_t* payload;
    };


    // Used to unpack clients info from response payload
    struct UnpackClient {
        ClientID clientId;
        uint8_t name[NAME_SIZE];

        UnpackClient() : name{ 0 } {}
    };


    // Used to unpack clients info from response payload
    struct UnpackMessage {
        ClientID clientId;
        uint32_t messageID;
        uint8_t msgType;
        uint32_t msgSize;

        UnpackMessage() : messageID(0), msgType(NONE_MESSAGE) , msgSize(0) {}
    };
#pragma pack(pop)