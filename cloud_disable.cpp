#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "cloud_disable.hpp"
#include "utility.hpp"

using namespace std;

bool checkAppsBlock(const string &buffer)
{
    return buffer.find("apps") != string::npos;
}

string extractGameIds(const string &libraryBuffer)
{
    stringstream id_buffer;
    string line;
    bool appsLineReached = false;

    if (!checkAppsBlock(libraryBuffer))
    {
        cout << "\"apps\" was not found in /libraryfolders.vdf, this can occur if you recently installed Steam" << endl;
        return "";
    }

    stringstream library_buffer(libraryBuffer);
    while (getline(library_buffer, line))
    {
        if (line.find("apps") != string::npos)
        {
            appsLineReached = true;
        }

        if (appsLineReached && any_of(line.begin(), line.end(), ::isdigit))
        {
            line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
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
            id_buffer << idLine << endl;
        }

        if (line.find("}") != string::npos && appsLineReached)
        {
            appsLineReached = false;
        }
    }

    return id_buffer.str();
}

bool replaceAppsBlock(const string &sharedConfigFile, const string &sharedConfigBuffer, const string &idBuffer)
{
    ifstream sharedconfig_if(sharedConfigFile);
    if (!sharedconfig_if)
    {
        throw runtime_error("Failed to open file: " + sharedConfigFile);
    }

    stringstream sharedconfig_buffer;
    string line;
    bool appsBlockReached = false;

    // Change this ########
    if (!checkAppsBlock(sharedConfigBuffer))
    {
        cout << "\"apps\" was not found in /sharedconfig.vdf, this can occur if you recently installed Steam" << endl;
        sharedconfig_if.close();
        return false;
    }

    while (getline(sharedconfig_if, line))
    {
        if (line.find("apps") != string::npos)
        {
            appsBlockReached = true;
            sharedconfig_buffer << line << endl;
            sharedconfig_buffer << "\t\t\t\t{" << endl;
            stringstream id_buffer(idBuffer);
            string sharedconfig_idLine;
            while (getline(id_buffer, sharedconfig_idLine))
            {
                sharedconfig_buffer << "\t\t\t\t\t" << sharedconfig_idLine << endl;
                sharedconfig_buffer << "\t\t\t\t\t{" << endl;
                sharedconfig_buffer << "\t\t\t\t\t\t\"CloudEnabled\"\t\t\"0\"" << endl;
                sharedconfig_buffer << "\t\t\t\t\t}" << endl;
            }
        }
        else if (appsBlockReached && line == "\t\t\t\t}")
        {
            appsBlockReached = false;
            sharedconfig_buffer << line << endl;
        }
        else if (!appsBlockReached)
        {
            sharedconfig_buffer << line << endl;
        }
    }

    sharedconfig_if.close();

    ofstream sharedconfig_of(sharedConfigFile);
    if (!sharedconfig_of)
    {
        throw runtime_error("Failed to open file for writing: " + sharedConfigFile);
    }
    sharedconfig_of << sharedconfig_buffer.str();
    sharedconfig_of.close();

    return true;
}
