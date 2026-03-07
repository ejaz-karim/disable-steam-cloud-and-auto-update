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
    std::string readFileContents(const std::string &filePath);
    std::string getAcfID(const std::string &path);
    std::string resolveSteamRoot();
    std::string promptSteamRoot();
    void saveSteamRoot(const std::string &path);
    std::string loadSteamRoot();
};
