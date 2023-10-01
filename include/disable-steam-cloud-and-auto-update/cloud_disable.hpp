#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

bool checkAppsBlock(const std::string &buffer);
std::string extractGameIds(const std::string &libraryBuffer);
bool replaceAppsBlock(const std::string &sharedConfigFile, const std::string &sharedConfigBuffer, const std::string &idBuffer);
