#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "disable-steam-cloud-and-auto-update/cloud_disable.hpp"
#include "disable-steam-cloud-and-auto-update/autoupdate_disable.hpp"
#include "disable-steam-cloud-and-auto-update/utility.hpp"
#include "disable-steam-cloud-and-auto-update/api.hpp"

using namespace std;

int main()
{
    try
    {
        bool restart = true;
        while (restart)
        {
            cout << ">Enter 1 to disable Steam Cloud" << endl;
            cout << ">Enter 2 to disable Auto-updates" << endl;
            cout << ">Select your option: " << endl;
            string input;
            getline(cin, input);
            if (input == "1")
            {
                restart = false;

                string defaultWinPath = "C:\\Program Files (x86)\\Steam\\userdata";

                string sharedconfig_directory = getDirectory(">Enter directory for sharedconfig.vdf:\n");
                string sharedconfig_file = sharedconfig_directory + "/sharedconfig.vdf";

                string library_directory = getDirectory(">Enter directory for libraryfolders.vdf:\n");
                string library_file = library_directory + "/libraryfolders.vdf";

                string sharedconfig_content = readFileContents(sharedconfig_file);
                string library_content = readFileContents(library_file);

                string game_ids = extractGameIds(library_content);

                stringstream test = removeQuotes(game_ids);
                stringstream appIdsNoQuotes = removeQuotes(game_ids);
                // apiRequest(appIdsNoQuotes);

                if (!game_ids.empty())
                {
                    if (replaceAppsBlock(sharedconfig_file, sharedconfig_content, game_ids))
                    {
                        cout << ">Success" << endl;
                    }
                }
            }
            else if (input == "2")
            {
                restart = false;
                string steamapps_directory = getDirectory(">Enter directory for steamapps:\n");
                bool iterateVar = iterateSteamApps(steamapps_directory);

                if (iterateVar)
                {
                    cout << ">Success" << endl;
                }
            }
            else
            {
                cout << "\n>Invalid input\n" << endl;
            }
        }
        cout << ">Press ENTER to exit..." << endl;
        getchar();
        return 0;
    }
    catch (const exception &e)
    {
        cerr << ">Error: " << e.what() << endl;
        return 1;
    }
}

