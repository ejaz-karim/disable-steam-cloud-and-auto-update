#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#ifndef UTILITY_HPP
#define UTILITY_HPP

std::string getDirectory(const std::string &prompt);
std::string readFileContents(const std::string &filePath);

#endif // UTILITY_HPP
