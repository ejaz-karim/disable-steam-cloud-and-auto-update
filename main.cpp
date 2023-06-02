#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "cloud_disable.hpp"
#include "autoupdate_disable.hpp"
#include "utility.hpp"

using namespace std;

int main()
{
    try
    {
        bool restart = true;
        while (restart)
        {
            cout << "Enter 1 for Cloud disable, 2 for Autoupdate disable" << endl;
            string input;
            getline(cin, input);
            if (input == "1")
            {
                restart = false;
                string sharedconfig_directory = getDirectory("Enter directory for sharedconfig.vdf: ");
                string sharedconfig_file = sharedconfig_directory + "/sharedconfig.vdf";

                string library_directory = getDirectory("Enter directory for libraryfolders.vdf: ");
                string library_file = library_directory + "/libraryfolders.vdf";

                string sharedconfig_content = readFileContents(sharedconfig_file);
                string library_content = readFileContents(library_file);

                string game_ids = extractGameIds(library_content);

                if (!game_ids.empty())
                {
                    if (replaceAppsBlock(sharedconfig_file, sharedconfig_content, game_ids))
                    {
                        cout << "Success." << endl;
                    }
                }
            }
            else if (input == "2")
            {
                restart = false;
                string steamapps_directory = getDirectory("Enter directory for steamapps: ");
                bool iterateVar = iterateSteamApps(steamapps_directory);

                if (iterateVar)
                {
                    cout << "Success." << endl;
                }
            }
            else
            {
                cout << "Invalid input." << endl;
            }
        }

        cout << "Press ENTER to exit..." << endl;
        getchar();
        return 0;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}
