#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "disable-steam-cloud-and-auto-update/cloud_disable.hpp"
#include "disable-steam-cloud-and-auto-update/autoupdate_disable.hpp"
#include "disable-steam-cloud-and-auto-update/utility.hpp"
#include "disable-steam-cloud-and-auto-update/api.hpp"

using namespace std;

int main()
{
    try
    {
        while (true)
        {
            cout << ">Enter 1 to disable Steam Cloud" << endl;
            cout << ">Enter 2 to disable Auto-updates" << endl;
            cout << ">Select your option: " << endl;
            string input;
            getline(cin, input);
            if (input == "1")
            {
                FileUtility fileUtility;
                CloudDisabler cloudDisabler;
                // Api api;

                string userDataPath = "C:/Program Files (x86)/Steam/userdata";
                string steamAppsPath = "C:/Program Files (x86)/Steam/steamapps";

                if (fileUtility.checkPathExists(userDataPath) && fileUtility.checkPathExists(steamAppsPath))
                {
                    string library_file = steamAppsPath + "/libraryfolders.vdf";
                    string library_content = fileUtility.readFileContents(library_file);
                    string acfIds = fileUtility.getAcfID(steamAppsPath);

                    for (const auto &entry : filesystem::directory_iterator(userDataPath))
                    {
                        if (entry.is_directory())
                        {
                            string steamID = entry.path().string();
                            string remotePath = steamID + "/7/remote";
                            if (filesystem::exists(remotePath))
                            {
                                string sharedConfigPath = remotePath + "/sharedconfig.vdf";
                                string sharedConfigText  = fileUtility.readFileContents(sharedConfigPath);

                                if (!acfIds.empty())
                                {
                                    if (cloudDisabler.replaceAppsBlock(sharedConfigPath, sharedConfigText, acfIds))
                                    {
                                        cout << ">Success" << endl;
                                    }
                                }
                            }
                            else
                            {
                                cout << "Could not find expected /remote/ folder given this path: " << remotePath << endl;
                            }
                        }
                    }

                    break;
                }
                else
                {
                    string sharedconfig_directory = fileUtility.getDirectory("\n>Enter directory for sharedconfig.vdf:\nExample: C:/Program Files (x86)/Steam/userdata/STEAM ID/7/remote");
                    string sharedConfigPath = sharedconfig_directory + "/sharedconfig.vdf";

                    string library_directory = fileUtility.getDirectory("\n>Enter directory for libraryfolders.vdf:\nExample: C:/Program Files (x86)/Steam/steamapps");
                    string library_file = library_directory + "/libraryfolders.vdf";

                    string sharedConfigText = fileUtility.readFileContents(sharedConfigPath);
                    string library_content = fileUtility.readFileContents(library_file);

                    string acfIds = fileUtility.getAcfID(steamAppsPath);

                    // stringstream appIdsNoQuotes = api.removeQuotes(game_ids);
                    // apiRequest(appIdsNoQuotes);

                    if (!acfIds.empty())
                    {
                        if (cloudDisabler.replaceAppsBlock(sharedConfigPath, sharedConfigText, acfIds))
                        {
                            cout << ">Success" << endl;
                        }
                    }
                    break;
                }
            }
            else if (input == "2")
            {
                FileUtility fileUtility;
                AutoUpdateDisabler autoUpdateDisabler;

                string steamAppsPath = "C:\\Program Files (x86)/Steam/steamapps";

                if (fileUtility.checkPathExists(steamAppsPath))
                {
                    bool iterateSteamAppsPath = autoUpdateDisabler.iterateSteamApps(steamAppsPath);
                    if (iterateSteamAppsPath)
                    {
                        cout << ">Success" << endl;
                    }
                }
                else
                {
                    cout << "Could not find Steam directory in Program Files (x86)\n"
                         << endl;
                    string manualSteamAppsPath = fileUtility.getDirectory(">Enter directory for /Steam/steamapps/ :\n");
                    bool iterateManualPath = autoUpdateDisabler.iterateSteamApps(manualSteamAppsPath);
                    if (iterateManualPath)
                    {
                        cout << ">Success" << endl;
                    }
                }
                break;
            }
            else
            {
                cout << "\n>Invalid input\n"
                     << endl;
            }
        }
        cout << ">Press ENTER to exit..." << endl;
        getchar();
    }
    catch (const exception &e)
    {
        cerr << ">Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
