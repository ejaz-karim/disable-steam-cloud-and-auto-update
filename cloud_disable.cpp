#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

using namespace std;

int main()
{
    string sharedconfig_directory;
    string library_directory;

    // C:\Program Files (x86)\Steam\userdata\<id>\7\remote is the directory in Windows
    cout << "Enter directory for sharedconfig.vdf: ";
    getline(cin, sharedconfig_directory);
    replace(sharedconfig_directory.begin(), sharedconfig_directory.end(), '\\', '/'); // Windows uses \ instead of / for directories which need to be switched.
    string sharedconfig_file = sharedconfig_directory + "/sharedconfig.vdf";          // Creates directory to the main config file

    // Input stream class to operate on the library folders file which has all the game ids
    cout << "Enter directory for libraryfolders.vdf: ";
    getline(cin, library_directory);
    replace(library_directory.begin(), library_directory.end(), '\\', '/');
    string library_file = library_directory + "/libraryfolders.vdf";
    ifstream library_if(library_file);

    stringstream library_buffer;          // Create a stringstream object which can be used to create a buffer to store information in
    library_buffer << library_if.rdbuf(); // Add the information from the library ifstream to the library buffer

    stringstream id_buffer;       // Create a stringstream object to store only the ids from the library_buffer
    string line;                  // String used to temporarily store each line of the library in the while loop
    bool appsLineReached = false; // Switch which tells us when the line where "apps" is found has been reached

    if (library_buffer.str().find("apps") == string::npos)
    {
        cout << "\"apps\" was not found in /libraryfolders.vdf, this can occur if you recently installed steam" << endl;
        cout << "Press ENTER to exit..." << endl;
        getchar();
        library_if.close();
        return 0;
    }
    else
    {
        // While loop to go through each line in the library
        while (getline(library_buffer, line))
        {

            // if the "apps" line is found (not equal to no position), then turn the bool switch on
            if (line.find("apps") != string::npos)
            {
                appsLineReached = true;
            }

            // If the apps switch is on and the line has any digits then we enter the if statement
            if (appsLineReached == true && any_of(line.begin(), line.end(), ::isdigit))
            {
                // Remove all the spaces in the line to give it this format -> "id""number"
                line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
                // Loop through each line to extract only the game id numbers. Format = "id""number" so we want to loop from i = 1 until the first " is found
                string idLine;
                int quoteCount = 0;
                for (int i = 0; quoteCount < 2; ++i)
                {
                    if (line[i] == '\"')
                    {
                        ++quoteCount;
                    }
                    idLine += line[i];
                }
                id_buffer << idLine << endl; // add the id to an apps buffer
            }

            // Assuming we are in the apps block, if the find the } closing bracket we turn the apps switch off
            if (line.find("}") != string::npos && appsLineReached == true)
            {
                appsLineReached = false;
            }
        }
    }

    // Outputs the ids to the terminal
    // cout << id_buffer.str() << endl;

    ifstream sharedconfig_if(sharedconfig_file); // Input file stream created for the main config
    stringstream sharedconfig_buffer;            // Stringstream buffer which we will use to replace the original config

    bool appsBlockReached = false;

    if (sharedconfig_buffer.str().find("apps") == string::npos)
    {
        cout << "\"apps\" was not found in /sharedconfig.vdf, this can occur if you recently installed steam" << endl;
        cout << "Press ENTER to exit..." << endl;
        getchar();
        library_if.close();
        sharedconfig_if.close();
        return 0;
    }
    else
    {
        // While loop to go through each line from the main config and add it to the sharedconfig_buffer accordingly
        while (getline(sharedconfig_if, line))
        {
            // If apps block has been reached
            if (line.find("apps") != string::npos)
            {
                appsBlockReached = true;
                // Add all the lines from the original config in the same structure and format without the things within
                // the { } block. This is to ensure that we dont have repeats of information and we are only adding the id of
                // games found from the library folder we entered earlier.
                sharedconfig_buffer << line << endl;
                sharedconfig_buffer << "\t\t\t\t{" << endl;

                // By sticking to the format of the config, we make all ids CloudEnabled = 0
                string sharedconfig_idLine;
                while (getline(id_buffer, sharedconfig_idLine))
                {
                    sharedconfig_buffer << "\t\t\t\t\t" << sharedconfig_idLine << endl;
                    sharedconfig_buffer << "\t\t\t\t\t{" << endl;
                    sharedconfig_buffer << "\t\t\t\t\t\t\"CloudEnabled\"\t\t\"0\"" << endl;
                    sharedconfig_buffer << "\t\t\t\t\t}" << endl;
                }
            }

            // If we reached the closing bracket line and we are still within the apps block, turn off the app block switch and add this line
            else if (appsBlockReached && line == "\t\t\t\t}")
            {
                appsBlockReached = false;
                sharedconfig_buffer << line << endl;
            }

            // if we are not inside the apps block we want to add all the lines to make sure everything outside of what we are editing is added
            else if (appsBlockReached == false)
            {
                sharedconfig_buffer << line << endl;
            }
        }
    }

    // Output file stream created so that we can add the new config we have created and save it replacing the old config
    ofstream sharedconfig_of(sharedconfig_file);
    sharedconfig_of << sharedconfig_buffer.str();

    // Shows us the state of the config file after running this script
    // cout << sharedconfig_buffer.str() << endl;

    library_if.close();
    sharedconfig_if.close();
    sharedconfig_of.close();
    cout << "Success." << endl;
    cout << "Press ENTER to exit..." << endl;
    getchar();
    return 0;
}