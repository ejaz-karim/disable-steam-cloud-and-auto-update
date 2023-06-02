#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#ifndef AUTOUPDATE_DISABLE_HPP
#define AUTOUPDATE_DISABLE_HPP

bool checkUpdateBehaviour(const std::string &buffer);
std::string replaceUpdateBehaviour();
bool iterateSteamApps(const std::string &steamAppsDirectory);

#endif // AUTOUPDATE_DISABLE_HPP