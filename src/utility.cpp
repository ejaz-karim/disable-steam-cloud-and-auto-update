#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "utility.hpp"

using namespace std;

string getDirectory(const string &prompt)
{
    bool restart = true;
    while (restart)
    {
        string directory;
        cout << prompt;
        getline(cin, directory);
        if (filesystem::exists(directory))
        {
            restart = false;
            replace(directory.begin(), directory.end(), '\\', '/');
            return directory;
        }
        else
        {
            cout << ">The directory you entered doesn't exist. Please try again." << endl;
        }
    }
    return ""; // Default return statement, will not be reached
}

string readFileContents(const string &filePath)
{
    ifstream file(filePath);
    if (!file)
    {
        throw runtime_error("Failed to open file: " + filePath);
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}
