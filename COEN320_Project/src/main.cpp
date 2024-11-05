#include <iostream>
#include <fstream>
#include "ATC.h"


int main() {
    std::cout << "ATC system starting..." << std::endl;

	std::ifstream inputFile("/input.txt");
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open input.txt file!" << std::endl;
        return 1;  // Exit if the file is not found
    }

    // message after opening the file successfully
    std::cout << "input.txt file opened successfully." << std::endl;

    // Create ATC system and start
    ATC atc;

    // message before starting the ATC system
    std::cout << "Starting the ATC system..." << std::endl;

    atc.start();

    // message after ATC system finishes
    std::cout << "ATC system finished." << std::endl;

    return 0;
}
