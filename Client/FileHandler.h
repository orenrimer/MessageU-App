#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>



class FileHandler {
public:
	FileHandler() : m_fs(nullptr), m_isOpen(false) {}
	bool write(const std::string&, const std::string&);
	bool readLine(const std::string&, std::string&, bool=false);
	bool fileExists(const std::string&);
	void closeFS();
private:
	bool openFile(const std::string&, bool=false);
	std::fstream* m_fs;
	bool m_isOpen;
};


