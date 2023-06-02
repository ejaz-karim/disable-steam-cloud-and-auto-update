#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "autoupdate_disable.hpp"
#include "utility.hpp"

using namespace std;

bool checkUpdateBehaviour(const string &buffer)
{
    return buffer.find("AutoUpdateBehavior") != string::npos;
}

bool replaceUpdateBehaviour()
{

}

string iterateSteamApps(const string &steamAppsDirectory)
{
    for(const auto& directory : )

}
