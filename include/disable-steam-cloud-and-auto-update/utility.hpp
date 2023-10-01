#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

std::string getDirectory(const std::string &prompt);
std::string readFileContents(const std::string &filePath);
