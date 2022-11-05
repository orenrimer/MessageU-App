#include "SocketHandler.h"
#include <iostream>
#include <boost/algorithm/string/trim.hpp>



SocketHandler::SocketHandler() : m_io_context(nullptr), m_resolver(nullptr), m_sock(nullptr), m_isConnected(false), m_fileHandler(nullptr) {
    getServeInfo();
}


SocketHandler::~SocketHandler(){
    m_address.clear();
    m_port.clear();
    delete m_fileHandler;
    closeSocket();
}


bool SocketHandler::connect() {
    if (!isValidInfo(m_address, m_port)) return false;

    try {
        if (m_isConnected) closeSocket();
        m_io_context = new boost::asio::io_context;
        m_sock = new tcp::socket(*m_io_context);
        m_resolver = new tcp::resolver(*m_io_context);
        boost::asio::connect(*m_sock, m_resolver->resolve(m_address, m_port));
        m_sock->non_blocking(false);
        m_isConnected = true;
    } catch(...) {
        closeSocket();
    }

    return m_isConnected;
}



bool SocketHandler::write(const uint8_t* reqBuffer, const size_t size) {
    if (reqBuffer == nullptr || size == 0) return false;

    try {
        boost::system::error_code error;
        uint32_t bytesLeft = size;
        uint32_t offset = 0;

        while (bytesLeft > 0) {
            uint8_t temp[PACKET_SIZE] = { 0 };
            uint32_t toWrite = std::min(bytesLeft, PACKET_SIZE);
            memcpy(temp, &reqBuffer[offset], toWrite);

            if (checkBigEndian()) ReverseBytes(temp, PACKET_SIZE);

            boost::asio::write(*m_sock, boost::asio::buffer(temp, PACKET_SIZE));
            offset += PACKET_SIZE;
            bytesLeft -= toWrite;
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}




bool SocketHandler::read(uint8_t* buffer, const size_t size) {
    if (size == 0 || buffer == nullptr) return false;

    size_t bytesLeft = size;
    uint8_t* ptr = buffer;
    boost::system::error_code errorCode;

    try {
        while (bytesLeft > 0) {
            uint8_t tempBuffer[PACKET_SIZE] = { 0 };

            size_t bytesRead = boost::asio::read(*m_sock, boost::asio::buffer(tempBuffer, PACKET_SIZE), errorCode);
            if (bytesRead == 0) {
                std::cout << "1" << std::endl;
                return false;
            }
            // It's required to convert from little endian to big endian.
            if (checkBigEndian()) ReverseBytes(tempBuffer, bytesRead);
        
            const size_t bytesToCopy = (bytesLeft > bytesRead) ? bytesRead : bytesLeft;  // prevent buffer overflow.
            memcpy(ptr, tempBuffer, bytesToCopy);
            ptr += bytesToCopy;
            bytesLeft = (bytesLeft < bytesToCopy) ? 0 : (bytesLeft - bytesToCopy);   // unsigned protection.
        }
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}




void SocketHandler::swapBytes(uint8_t* const buffer, size_t size) const
{
    if (buffer == nullptr || size < sizeof(uint32_t))
        return;

    size -= (size % sizeof(uint32_t));
    uint32_t* const ptr = reinterpret_cast<uint32_t* const>(buffer);
    for (size_t i = 0; i < size; ++i)
    {
        const uint32_t tmp = ((buffer[i] << 8) & 0xFF00FF00) | ((buffer[i] >> 8) & 0xFF00FF);
        buffer[i] = (tmp << 16) | (tmp >> 16);
    }

}






bool SocketHandler::getServeInfo() {
    if (!m_fileHandler) m_fileHandler = new FileHandler;


    std::string info;
    if (!m_fileHandler->readLine(SERVER_INFO_PATH, info, true)) {
        std::cout << "Error while trying to read '" << SERVER_INFO_PATH << std::endl;
        return false;
    }

    boost::algorithm::trim(info);

    const auto pos = info.find(':');
    if (pos == std::string::npos) {
        std::cout << "Error in file '" << SERVER_INFO_PATH << "' invalid format, missing separator ':'";
        return false;
    }
    const auto address = info.substr(0, pos);
    const auto port = info.substr(pos + 1);

    if (!isValidInfo(address, port)) {
        std::cout << "Invalid IP address or port, can not connect to server!";
        return false;
    }

    m_address = address;
    m_port = port;
    return true;
}


bool SocketHandler::isValidInfo(const std::string& address, const std::string& port) {

    // check valid address
    if ((address != "localhost") && (address != "LOCALHOST")) {
        try {
            (void)boost::asio::ip::address_v4::from_string(address);
        } catch (...) {
            return false;
        }
    }
  
   // check valid port
    try {
        const int p = std::stoi(port);
        return (p != 0);  // port 0 is invalid..
    } catch (...) {
        return false;
    }
}


bool SocketHandler::socketWrapper(const uint8_t* reqBuffer, const size_t reqSize, uint8_t* const respBuffer, const size_t resSize, bool close) {
    if (!connect()) {
        return false;
    }
    if (!write(reqBuffer, reqSize)){
        closeSocket();
        return false;
    }
    if (!read(respBuffer, resSize)){
        closeSocket();
        return false;
    }

    if(close) closeSocket();
    return true;
}




void SocketHandler::closeSocket() {
    try {
        if (m_sock != nullptr) m_sock->close();
    } catch (...) {
       /**/
    }

    delete m_io_context;
    m_io_context = nullptr;
    delete m_resolver;
    m_resolver = nullptr;
    delete m_sock;
    m_sock = nullptr;
    m_isConnected = false;
}



bool SocketHandler::checkBigEndian() {
    int n = 1;
    // little endian if true
    return !(*(char*)&n == 1);
}



void SocketHandler::ReverseBytes(uint8_t* buffer, size_t size) {
    char* istart = reinterpret_cast<char* const>(buffer);
    char *iend = istart + size;
    std::reverse(istart, iend);
}