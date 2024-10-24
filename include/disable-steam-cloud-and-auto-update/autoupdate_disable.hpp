#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

class AutoUpdateDisabler
{
public:
    AutoUpdateDisabler();
    bool checkUpdateBehaviour(const std::string &buffer);
    bool checkPathExists(const string &path);
    std::string replaceUpdateBehaviour();
    bool iterateSteamApps(const std::string &steamAppsDirectory);
};
