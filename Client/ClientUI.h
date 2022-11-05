#pragma once
#include <iostream>
#include <string>
#include <set>
#include <map>
#include "Protocol.h"
#include <boost/algorithm/string.hpp>



class ClientUI{
public:
	enum class MenuOption {
		REGISTER = 10,
		GET_CLIENT_LIST = 20,
		GET_PUBLIC_KEY = 30,
		GET_UNREAD_MSG = 40,
		SEND_MSG = 50,
		REQ_SYM_KEY = 51,
		SEND_SYM_KEY = 52,
		SEND_FILE = 53,
		EXIT = 0,
		NONE_OPTION = -1
	};

	void displayMainMenu();
	MenuOption display(bool = false);
	const std::string getCleanInput(const std::string&);


private:
	bool isValidOption(const int, MenuOption&);
	void clearScreen() const { system("cls"); };


	std::set<MenuOption> m_validOptions{
		MenuOption::REGISTER ,
		MenuOption::GET_CLIENT_LIST ,
		MenuOption::GET_CLIENT_LIST,
		MenuOption::GET_PUBLIC_KEY,
		MenuOption::GET_UNREAD_MSG,
		MenuOption::SEND_MSG,
		MenuOption::REQ_SYM_KEY,
		MenuOption::SEND_SYM_KEY,
		MenuOption::SEND_FILE,
		MenuOption::EXIT
	};
};