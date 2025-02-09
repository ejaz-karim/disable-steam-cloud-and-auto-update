#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "disable-steam-cloud-and-auto-update/cloud_disable.hpp"

using namespace std;

CloudDisabler::CloudDisabler() {}

bool CloudDisabler::checkAppsBlock(const string &buffer)
{
    return buffer.find("\"apps\"") != string::npos;
}

void CloudDisabler::createAppsBlock()
{


}

[[deprecated("Use getAcfID() in utility.cpp instead.")]]
string CloudDisabler::extractGameIds(const string &libraryBuffer)
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







bool CloudDisabler::replaceAppsBlock(const string &sharedConfigPath, const string &sharedConfigText, const string &acfIds)
{
    ifstream sharedconfig_if(sharedConfigPath);
    if (!sharedconfig_if)
    {
        throw runtime_error("Failed to open file: " + sharedConfigPath);
    }

    stringstream sharedconfig_buffer;
    string line;

    if (!checkAppsBlock(sharedConfigText))
    {

        string nextLine;

        while (getline(sharedconfig_if, line))
        {
            if (getline(sharedconfig_if, nextLine))
            {

                sharedconfig_buffer << line << endl;
                sharedconfig_buffer << nextLine << endl;

                if (line == "\t\t\t\"Steam\"" && nextLine == "\t\t\t{")
                {

                    sharedconfig_buffer << "\t\t\t\t\"apps\"" << endl;
                    sharedconfig_buffer << "\t\t\t\t{" << endl;
                    sharedconfig_buffer << "\t\t\t\t}" << endl;
                }
            }
        }

        ofstream sharedconfig_of(sharedConfigPath);
        sharedconfig_of << sharedconfig_buffer.str();

        // sharedconfig_of.close();
        // sharedconfig_if.close();
        
        // sharedconfig_buffer.str("");
        // sharedconfig_buffer.clear();
        // cout << "############" << endl;

    }


    bool appsBlockReached = false;
    // cout << "+++++++++++++++++++" << endl;

    while (getline(sharedconfig_if, line))
    {
        // cout << "@@@@@@@@@@@@" << endl;
        if (line.find("\"apps\"") != string::npos)
        {
            appsBlockReached = true;
            sharedconfig_buffer << line << endl;
            sharedconfig_buffer << "\t\t\t\t{" << endl;
            stringstream id_buffer(acfIds);
            string sharedconfig_idLine;
            while (getline(id_buffer, sharedconfig_idLine))
            {
                sharedconfig_buffer << "\t\t\t\t\t" << sharedconfig_idLine << endl;
                sharedconfig_buffer << "\t\t\t\t\t{" << endl;
                sharedconfig_buffer << "\t\t\t\t\t\t\"CloudEnabled\"\t\t\"0\"" << endl;
                sharedconfig_buffer << "\t\t\t\t\t}" << endl;
            }
        }
        else if (line == "\t\t\t\t}")
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

    ofstream sharedconfig_of(sharedConfigPath);
    if (!sharedconfig_of)
    {
        throw runtime_error("Failed to open file for writing: " + sharedConfigPath);
    }
    sharedconfig_of << sharedconfig_buffer.str();
    sharedconfig_of.close();

    return true;
}
