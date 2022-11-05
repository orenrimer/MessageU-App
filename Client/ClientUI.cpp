#pragma once
#include "ClientUI.h"



void ClientUI::displayMainMenu() {
    std::cout  << "\n\nPlease select one of the options below:\n\t"
        "10) Register\n\t"
        "20) Request for clients list\n\t"
        "30) Request for public key\n\t"
        "40) Request for unread messages\n\t"
        "50) Send a text message\n\t"
        "51) Send a request for symmetric key\n\t"
        "52) Send your symmetric key\n\t"
        "0) Exit client\n"
        "Please select one of the options above: " 
    << std::endl;
}



ClientUI::MenuOption ClientUI::display(bool clear) {
    if (clear) {
        clearScreen();
    }

    displayMainMenu();

    int choice;
    MenuOption option;
    std::cin >> choice;
    bool validOption = isValidOption(choice, option);
    if (!validOption) return ClientUI::MenuOption::NONE_OPTION;
    return option;
}



bool ClientUI::isValidOption(const int choice, MenuOption& outOption) {
    for (const auto& opt : m_validOptions) {
        if (choice == static_cast<uint32_t>(opt)) {
            outOption = opt;
            return true;
        }
    }
    return false;
}



const std::string ClientUI::getCleanInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;

    do {
        std::getline(std::cin, input);
        boost::algorithm::trim(input);
     
    } while (input.empty());

    return input;
}

