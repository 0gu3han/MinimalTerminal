#include <fstream>
#include <iostream>
#include <string>

import presentation;

auto main(int argc, char* argv[]) -> int {
    // Read the file name from argv
    if (argc < 2) {
        std::cerr << "No presentation file given\n";
        return 1;
    }
    std::string fileName = argv[1];

    // Open a file stream
    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "Could not open " << fileName << "\n";
        return 1;
    }

    // Pass fstream to Presentation constructor
    Presentation presentation(file);

    // Close the filestream
    file.close();

    // Invoke Presentation.start() method
    presentation.start();

    return 0;
}
