#pragma once
#include <boost/asio.hpp>
#include <algorithm>
#include "FileHandler.h"


using boost::asio::ip::tcp;

constexpr size_t PACKET_SIZE = 1024;
constexpr auto SERVER_INFO_PATH = "server.info";


class SocketHandler {
public:
	SocketHandler();
	~SocketHandler();

	bool socketWrapper(const uint8_t*, const size_t, uint8_t* const, const size_t, bool=true);
	bool connect();
	bool write(const uint8_t*, const size_t);
	bool read(uint8_t*, const size_t);
	void closeSocket();
	bool isConnected() { return m_isConnected; }
	bool getServeInfo();
	void swapBytes(uint8_t* const buffer, size_t size) const;
private:
	bool checkBigEndian();
	void ReverseBytes(uint8_t*, size_t);
	bool isValidInfo(const std::string&, const std::string&);
	FileHandler* m_fileHandler;
	std::string m_address;
	std::string m_port;
	boost::asio::io_context* m_io_context;
	tcp::socket* m_sock;
	tcp::resolver* m_resolver;
	bool m_isConnected;
};