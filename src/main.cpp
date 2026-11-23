#include <exception>
#include <fstream>
#include <iostream>
#include <string>

import presentation;

auto main(int argc, char *argv[]) -> int {
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

  // Read the slides before curses starts, so a malformed file is reported on
  // a normal terminal
  try {
    Presentation presentation(file, fileName);
    file.close();
    presentation.start();
  } catch (const std::exception &error) {
    std::cerr << fileName << ": " << error.what() << "\n";
    return 1;
  }

  return 0;
}
