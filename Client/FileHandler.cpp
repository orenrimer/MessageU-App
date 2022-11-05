#include "FileHandler.h"



bool FileHandler::openFile(const std::string& filePath, bool write) {
	try {
		if(m_isOpen) closeFS();
		m_fs = new std::fstream;
		const auto flags = write ? (std::fstream::binary | std::ios_base::app) : (std::fstream::binary | std::fstream::in);

		m_fs->open(filePath, flags);
		m_isOpen = m_fs->is_open();
	} catch (...) {
		m_isOpen = false;
	}
	return m_isOpen;
}



bool FileHandler::write(const std::string& filePath, const std::string& data) {
	if (data.empty()) return false;
	if (!openFile(filePath, true)) return false;

	std::string newline = data;
	newline.append("\n");		// make sure to null terminate
	bool success = true;

	try {
		m_fs->write(newline.c_str(), newline.size());
	}
	catch (...) {
		success = false;
	}

	closeFS();
	return success;
}


bool FileHandler::readLine(const std::string& filePath, std::string& outBuffer, bool lastLine) {
	if (!m_isOpen) openFile(filePath);

	if (m_fs == nullptr || !m_isOpen) return false;
	
	bool success = true;

	try {
		if (!std::getline(*m_fs, outBuffer)) success = false;
	} catch (...){
		success = false;
	}

	if (lastLine) closeFS();
	return success;
}



bool FileHandler::fileExists(const std::string& filePath) {
	return std::filesystem::exists(filePath);
}


void FileHandler::closeFS() {
	try {
		if (m_fs != nullptr)
			m_fs->close();
	}
	catch (...) {
		/* Do Nothing */
	}

	delete m_fs;
	m_fs = nullptr;
	m_isOpen = false;
}