#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

using namespace std;

int main()
{
    string input_directory;

    // C:\Program Files (x86)\Steam\userdata\<id>\7\remote is the directory in Windows
    cout << "Enter directory: ";
    getline(cin, input_directory);
    replace(input_directory.begin(), input_directory.end(), '\\', '/');

    string file_directory = input_directory + "/sharedconfig.vdf";

    ifstream if_file(file_directory);
    stringstream buffer;
    buffer << if_file.rdbuf();

    ifstream library("C:/Program Files (x86)/Steam/steamapps/libraryfolders.vdf");
    stringstream library_buffer;
    library_buffer << library.rdbuf();

    stringstream apps_buffer;

    string line;
    bool appsLineReached = false;
    while (getline(library_buffer, line))
    {
        if (line.find("apps") != string::npos)
        {
            appsLineReached = true;
        }
        if (appsLineReached == true && line.find("apps") == string::npos)
        {
            apps_buffer << line << endl;
            if (line.find("}") != string::npos)
            {
                appsLineReached = false;
            }
        }
    }
    cout << apps_buffer.str() << endl;

    regex pattern("\"CloudEnabled\"\t\t\"1\"");
    string replacement("\"CloudEnabled\"\t\t\"0\"");
    string result = regex_replace(buffer.str(), pattern, replacement);

    if_file.close();

    ofstream of_file(file_directory);
    of_file << result;
    of_file.close();

    return 0;
}