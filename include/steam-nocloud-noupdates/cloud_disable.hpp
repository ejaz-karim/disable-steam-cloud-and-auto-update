#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

class CloudDisabler
{
public:
    CloudDisabler();
    bool replaceAppsBlock(const std::string &sharedConfigPath, const std::string &sharedConfigText, const std::string &acfIds);
    bool iterateSharedConfig(const std::string &userDataPath, const std::string &acfIds);
};
