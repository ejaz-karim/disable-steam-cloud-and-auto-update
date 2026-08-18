#include <iostream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#include "steam-nocloud-noupdates/cloud_disable.hpp"
#include "steam-nocloud-noupdates/autoupdate_disable.hpp"
#include "steam-nocloud-noupdates/utility.hpp"

using namespace std;

int main()
{
#ifdef _WIN32
    SetConsoleTitleA("Steam NoCloud NoUpdates");
#endif

    try
    {
        FileUtility fileUtility;
        string steamRoot = fileUtility.resolveSteamRoot();
        string userDataPath = steamRoot + "/userdata";
        vector<string> libraryPaths = fileUtility.parseLibraryFolders(steamRoot);

        while (true)
        {
            cout << endl;
            cout << ">Enter 0 to change Steam directory" << endl;
            cout << ">Enter 1 to disable Steam Cloud for all games (per-game settings)" << endl;
            cout << ">Enter 2 to disable Auto-updates and Unschedule all game and Workshop updates" << endl;
            cout << ">Enter 3 to exit" << endl;
            cout << ">Select your option: " << endl;
            string input;
            cout << ">";
            getline(cin, input);
            cout << endl;

            if (input == "0")
            {
                steamRoot = fileUtility.promptSteamRoot();
                userDataPath = steamRoot + "/userdata";
                libraryPaths = fileUtility.parseLibraryFolders(steamRoot);
                cout << ">Steam directory updated to: " << steamRoot << endl;
            }
            else if (input == "1")
            {
                vector<int> combinedAcfIds;
                for (const string& lib : libraryPaths) {
                    string sAppsPath = lib + "/steamapps";
                    if (filesystem::exists(sAppsPath)) {
                        vector<int> ids = fileUtility.getAcfID(sAppsPath);
                        combinedAcfIds.insert(combinedAcfIds.end(), ids.begin(), ids.end());
                    }
                }

                string acfIds = fileUtility.sortAcfID(combinedAcfIds);
                if (acfIds.empty())
                {
                    cout << ">There are no games in your steamapps folders" << endl;
                    continue;
                }
                
                CloudDisabler cloudDisabler;
                cloudDisabler.iterateSharedConfig(userDataPath, acfIds);
                cout << ">Success" << endl;
            }
            else if (input == "2")
            {
                bool foundAny = false;
                AutoUpdateDisabler autoUpdateDisabler;
                
                for (const string& lib : libraryPaths) {
                    string sAppsPath = lib + "/steamapps";
                    if (filesystem::exists(sAppsPath)) {
                        cout << ">Processing directory: " << sAppsPath << endl;
                        autoUpdateDisabler.iterateSteamApps(sAppsPath);
                        foundAny = true;
                    }
                }
                
                if (!foundAny)
                {
                    cout << ">There are no games in your steamapps folders" << endl;
                    continue;
                }
                cout << ">Success" << endl;
            }
            else if (input == "3")
            {
                break;
            }
            else
            {
                cout << ">Invalid input" << endl;
            }
        }
    }
    catch (const exception &e)
    {
        cerr << ">Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
