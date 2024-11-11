#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "disable-steam-cloud-and-auto-update/utility.hpp"

using namespace std;

FileUtility::FileUtility() {}

string FileUtility::getDirectory(const string &prompt)
{
    while (true)
    {
        string directory;
        cout << prompt;
        getline(cin, directory);

        if (filesystem::exists(directory))
        {
            replace(directory.begin(), directory.end(), '\\', '/');
            return directory;
        }
        else
        {
            cout << ">The directory you entered doesn't exist. Please try again." << endl;
        }
    }
}

string FileUtility::readFileContents(const string &filePath)
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

bool FileUtility::checkPathExists(const string &path)
{
    return filesystem::exists(path);
}

//Get all game ids from the .acf file names in /steamapps/
string FileUtility::getAcfID(const string &path)
{
    stringstream buffer;
    for (const auto &entry : filesystem::directory_iterator(path))
    {
        if (entry.path().extension() == ".acf")
        {
            buffer << entry << endl;
        }
    }
    return buffer.str()
}
