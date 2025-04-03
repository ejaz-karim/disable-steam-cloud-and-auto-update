#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "disable-steam-cloud-and-auto-update/cloud_disable.hpp"

using namespace std;

CloudDisabler::CloudDisabler() {}

bool CloudDisabler::checkAppsBlock(const string &text)
{
    return text.find("\"apps\"") != string::npos;
}


string CloudDisabler::deleteAppsBlock(const string &sharedConfigText){

  	string line;
    stringstream buffer;
	stringstream sharedConfigTextStream(sharedConfigText);

	while(getline(sharedConfigTextStream, line)){
	if(line == "\"apps\""){
	skip = true;
	
	}
	if(!skip){
	buffer << line << endl;
	}
	if(skip && line == "\t\t\t\t}"){
	skip = false;
	}



	}
	
	return buffer;
	
}

string CloudDisabler::createAppsBlock(const string &sharedConfigText)
{
    string line;
    string nextLine;
    stringstream buffer;
    stringstream sharedConfigTextStream(sharedConfigText);

    while (getline(sharedConfigTextStream, line))
    {
        if (getline(sharedConfigTextStream, nextLine))
        {

            buffer << line << endl;
            buffer << nextLine << endl;

            if (line == "\t\t\t\"Steam\"" && nextLine == "\t\t\t{")
            {
                buffer << "\t\t\t\t\"apps\"" << endl;
                buffer << "\t\t\t\t{" << endl;
                buffer << "\t\t\t\t}" << endl;
            }
        }
        else
        {
            buffer << line << endl;
        }
    }

    return buffer.str();
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
    string newConfig = sharedConfigText;

    if(checkAppsBlock(sharedConfigText){

        newConfig = deleteAppsBlock(sharedConfigText);

    }else{
        newConfig = createAppsBlock(sharedConfigText);

    }

    string line;
    string nextLine;
    stringstream buffer;
    stringstream sharedConfigTextStream(newConfig);

    while (getline(sharedConfigTextStream, line))
    {

        if (getline(sharedConfigTextStream, nextLine))
        {

            buffer << line << endl;
            buffer << nextLine << endl;

            if (line == "\t\t\t\t\"apps\"" && nextLine == "\t\t\t\t{")
            {

                stringstream acfIdsStream(acfIds);
                string id;
                while (getline(acfIdsStream, id))
                {
                    buffer << "\t\t\t\t\t" + id << endl;
                    buffer << "\t\t\t\t\t{" << endl;
                    buffer << "\t\t\t\t\t\t\"CloudEnabled\"\t\t\"0\"" << endl;
                    buffer << "\t\t\t\t\t}" << endl;
                }
            }
        }
        else
        {
            buffer << line << endl;
        }
    }

    ofstream sharedConfigFile(sharedConfigPath, ios::trunc);

    if (!sharedConfigFile)
    {
        cerr << "Error: Could not open file " << sharedConfigPath << endl;
        return false;
    }

    sharedConfigFile << buffer.str();
    sharedConfigFile.close();

    return true;
}
