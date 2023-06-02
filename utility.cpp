#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "utility.hpp"

using namespace std;

string getDirectory(const string &prompt)
{
    string directory;
    cout << prompt;
    getline(cin, directory);
    replace(directory.begin(), directory.end(), '\\', '/');
    return directory;
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
