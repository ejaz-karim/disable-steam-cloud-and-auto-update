#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <vector>
#include "steam-tweak-tool/utility.hpp"

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
vector<int> FileUtility::getAcfID(const string &path)
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

    return intVector;
}

string FileUtility::sortAcfID(vector<int> &intVector)
{
    sort(intVector.begin(), intVector.end());
    intVector.erase(unique(intVector.begin(), intVector.end()), intVector.end());
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

        if (filesystem::is_regular_file(root + "/steam.exe"))
        {
            saveSteamRoot(root);
            cout << ">Steam path saved to steam_path.cfg" << endl;
            return root;
        }
        else if (filesystem::is_regular_file(root + "/steam.dll") || filesystem::is_directory(root + "/steamapps"))
        {
            string resolvedRoot = resolveMainRootFromLibraryDirectory(root);
            if (!resolvedRoot.empty() && filesystem::is_regular_file(resolvedRoot + "/steam.exe"))
            {
                saveSteamRoot(resolvedRoot);
                cout << ">Steam path saved to steam_path.cfg" << endl;
                return resolvedRoot;
            }
            else
            {
                cout << ">Could not resolve main Steam root from library directory. Make sure Steam is installed." << endl;
            }
        }
        else
        {
            cout << ">Invalid Steam directory. Could not find steam.exe, steam.dll, or steamapps/." << endl;
        }
    }
}

string FileUtility::resolveSteamRoot()
{
    string root;
    // 1. Saved config takes priority
    string saved = loadSteamRoot();
    if (!saved.empty() && filesystem::is_regular_file(saved + "/steam.exe"))
    {
        cout << ">Using saved Steam path: " << saved << endl;
        root = saved;
    }
    else
    {
        // 2. Check default Windows path
        string defaultPath = "C:/Program Files (x86)/Steam";
        if (filesystem::is_regular_file(defaultPath + "/steam.exe"))
        {
            cout << ">Found Steam at: " << defaultPath << endl;
            root = defaultPath;
        }
        else
        {
            // 3. Prompt the user
            cout << ">Could not find Steam root folder" << endl;
            root = promptSteamRoot();
        }
    }

    vector<string> libraries = parseLibraryFolders(root);
    cout << ">Detected Library Folders:" << endl;
    for (const string& lib : libraries) {
        cout << ">" << lib << endl;
    }

    return root;
}

vector<string> FileUtility::parseLibraryFolders(const string &steamRoot)
{
    vector<string> paths;
    paths.push_back(steamRoot);

    string vdfPath = steamRoot + "/steamapps/libraryfolders.vdf";
    if (filesystem::exists(vdfPath))
    {
        ifstream file(vdfPath);
        if (file)
        {
            string line;
            while (getline(file, line))
            {
                if (line.find("\"path\"") != string::npos)
                {
                    size_t firstQuote = line.find('"', line.find("\"path\"") + 6);
                    if (firstQuote != string::npos)
                    {
                        size_t secondQuote = line.find('"', firstQuote + 1);
                        if (secondQuote != string::npos)
                        {
                            string path = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                            replace(path.begin(), path.end(), '\\', '/');
                            size_t pos;
                            while ((pos = path.find("//")) != string::npos) {
                                path.replace(pos, 2, "/");
                            }
                            if (path != steamRoot && find(paths.begin(), paths.end(), path) == paths.end()) {
                                paths.push_back(path);
                            }
                        }
                    }
                }
            }
        }
    }
    return paths;
}

string FileUtility::resolveMainRootFromLibraryDirectory(const string &path)
{
    string vdfPaths[] = {
        path + "/libraryfolder.vdf",
        path + "/libraryfolders.vdf",
        path + "/steamapps/libraryfolder.vdf",
        path + "/steamapps/libraryfolders.vdf"
    };

    string foundVdf = "";
    for (const string& p : vdfPaths) {
        if (filesystem::exists(p)) {
            foundVdf = p;
            break;
        }
    }

    if (!foundVdf.empty())
    {
        ifstream file(foundVdf);
        if (file)
        {
            string line;
            while (getline(file, line))
            {
                if (line.find("\"launcher\"") != string::npos)
                {
                    size_t firstQuote = line.find('"', line.find("\"launcher\"") + 10);
                    if (firstQuote != string::npos)
                    {
                        size_t secondQuote = line.find('"', firstQuote + 1);
                        if (secondQuote != string::npos)
                        {
                            string launcherPath = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                            replace(launcherPath.begin(), launcherPath.end(), '\\', '/');
                            size_t pos;
                            while ((pos = launcherPath.find("//")) != string::npos) {
                                launcherPath.replace(pos, 2, "/");
                            }
                            
                            size_t lastSlash = launcherPath.find_last_of('/');
                            if (lastSlash != string::npos)
                            {
                                return launcherPath.substr(0, lastSlash);
                            }
                        }
                    }
                }
            }
        }
    }
    return "";
}
