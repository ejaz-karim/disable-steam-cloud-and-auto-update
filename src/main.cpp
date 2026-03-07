#include <iostream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#include "disable-steam-cloud-and-auto-update/cloud_disable.hpp"
#include "disable-steam-cloud-and-auto-update/autoupdate_disable.hpp"
#include "disable-steam-cloud-and-auto-update/utility.hpp"

using namespace std;

int main()
{
#ifdef _WIN32
    SetConsoleTitleA("disable-steam-cloud-and-auto-update");
#endif

    try
    {
        FileUtility fileUtility;
        string steamRoot = fileUtility.resolveSteamRoot();
        string userDataPath = steamRoot + "/userdata";
        string steamAppsPath = steamRoot + "/steamapps";

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
                steamAppsPath = steamRoot + "/steamapps";
                cout << ">Steam directory updated to: " << steamRoot << endl;
            }
            else if (input == "1")
            {
                CloudDisabler cloudDisabler;
                string acfIds = fileUtility.getAcfID(steamAppsPath);

                if (acfIds.empty())
                {
                    cout << ">There are no games in your steamapps folder" << endl;
                    continue;
                }

                for (const auto &entry : filesystem::directory_iterator(userDataPath))
                {
                    if (entry.is_directory())
                    {
                        string steamID = entry.path().string();
                        replace(steamID.begin(), steamID.end(), '\\', '/');
                        
                        string remotePath = steamID + "/7/remote";
                        if (filesystem::exists(remotePath))
                        {
                            string sharedConfigPath = remotePath + "/sharedconfig.vdf";
                            if (filesystem::exists(sharedConfigPath))
                            {
                                string sharedConfigText = fileUtility.readFileContents(sharedConfigPath);
                                cloudDisabler.replaceAppsBlock(sharedConfigPath, sharedConfigText, acfIds);
                            }
                        }
                    }
                }

                cout << ">Success" << endl;
            }
            else if (input == "2")
            {
                AutoUpdateDisabler autoUpdateDisabler;
                autoUpdateDisabler.iterateSteamApps(steamAppsPath);
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
