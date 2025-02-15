#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

class CloudDisabler
{
public:
    CloudDisabler();
    bool checkAppsBlock(const std::string &buffer);
    void createAppsBlock(const std::string &sharedConfigPath, const std::string &sharedConfigText);
    std::string extractGameIds(const std::string &libraryBuffer);
    bool replaceAppsBlock(const std::string &sharedConfigPath, const std::string &sharedConfigText, const std::string &acfIds);
};
