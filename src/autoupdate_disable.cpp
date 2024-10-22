#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "disable-steam-cloud-and-auto-update/autoupdate_disable.hpp"

using namespace std;
namespace fs = std::filesystem;

AutoUpdateDisabler::AutoUpdateDisabler() {}

bool checkUpdateBehaviour(const string &buffer)
{
    return buffer.find("AutoUpdateBehavior") != string::npos;
}

string replaceUpdateBehaviour()
{
    return "\t\"AutoUpdateBehavior\"\t\t\"1\"";
}

bool iterateSteamApps(const string &steamAppsDirectory)
{
    for (const auto &entry : fs::directory_iterator(steamAppsDirectory))
    {
        if (fs::is_regular_file(entry.status()) && entry.path().extension() == ".acf")
        {
            ifstream file(entry.path().string());
            stringstream buffer;
            string line;

            while (getline(file, line))
            {
                if (checkUpdateBehaviour(line))
                {
                    buffer << replaceUpdateBehaviour() << endl;
                }
                else
                {
                    buffer << line << endl;
                }
            }

            file.close();

            ofstream file_of(entry.path().string());
            if (!file_of)
            {
                throw runtime_error("Failed to open file for writing: " + entry.path().string());
            }
            file_of << buffer.str();
            file_of.close();
        }
    }
    return true;
}
