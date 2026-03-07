#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <vector>
#include "disable-steam-cloud-and-auto-update/utility.hpp"

using namespace std;

FileUtility::FileUtility() {}

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


// Get all game ids from the .acf file names in /steamapps/
string FileUtility::getAcfID(const string &path)
{
    vector<int> intVector;
    for (const auto &entry : filesystem::directory_iterator(path))
    {
        if (entry.path().extension() == ".acf")
        {
            string filename = entry.path().filename().string();
            // erasing appmanifest_
            filename.erase(0, 12);
            // erasing .acf
            filename.erase(filename.size() - 4);

            intVector.push_back(stoi(filename));
        }
    }

    sort(intVector.begin(), intVector.end());
    stringstream buffer;

    for (auto &entry : intVector)
    {
        string id = to_string(entry);
        id = "\"" + id + "\"";
        buffer << id << endl;
    }
    return buffer.str();
}


string FileUtility::loadSteamRoot()
{
    ifstream file("steam_path.cfg");
    if (file)
    {
        string path;
        getline(file, path);
        file.close();
        if (!path.empty() && filesystem::exists(path))
        {
            return path;
        }
    }
    return "";
}

void FileUtility::saveSteamRoot(const string &path)
{
    ofstream file("steam_path.cfg", ios::trunc);
    if (file)
    {
        file << path;
        file.close();
    }
}

string FileUtility::promptSteamRoot()
{
    while (true)
    {
        string root;
        cout << ">Enter the path to your Steam root folder: " << endl;
        cout << ">Example: C:/Program Files (x86)/Steam" << endl;
        cout << ">";
        getline(cin, root);
        cout << endl;

        replace(root.begin(), root.end(), '\\', '/');

        if (filesystem::exists(root + "/userdata") && filesystem::exists(root + "/steamapps"))
        {
            saveSteamRoot(root);
            cout << ">Steam path saved to steam_path.cfg" << endl;
            return root;
        }
        else
        {
            cout << ">Invalid Steam directory. Expected /userdata/ and /steamapps/ folders inside it." << endl;
        }
    }
}

string FileUtility::resolveSteamRoot()
{
    // 1. Saved config takes priority
    string saved = loadSteamRoot();
    if (!saved.empty())
    {
        cout << ">Using saved Steam path: " << saved << endl;
        return saved;
    }

    // 2. Check default Windows path
    string defaultPath = "C:/Program Files (x86)/Steam";
    if (filesystem::exists(defaultPath + "/userdata") && filesystem::exists(defaultPath + "/steamapps"))
    {
        cout << ">Found Steam at: " << defaultPath << endl;
        return defaultPath;
    }

    // 3. Prompt the user
    cout << ">Could not find Steam root folder" << endl;
    return promptSteamRoot();
}
