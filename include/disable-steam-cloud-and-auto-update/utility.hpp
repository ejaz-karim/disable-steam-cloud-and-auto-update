#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

class FileUtility
{
public:
    FileUtility();
    std::string getDirectory(const std::string &prompt);
    std::string readFileContents(const std::string &filePath);
    bool checkPathExists(const std::string &path);
    std::string getAcfID(const std::string &path);
    std::stringstream removeQuotes(const std::string &game_ids);
};
