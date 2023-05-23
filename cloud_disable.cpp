#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

using namespace std;

int main() {
    string input_directory;

    // C:\Program Files (x86)\Steam\userdata\<id>\7\remote is the directory in Windows
    cout << "Enter directory: ";
    getline(cin, input_directory);
    replace(input_directory.begin(), input_directory.end(), '\\', '/');
    string file_directory = input_directory + "/sharedconfig.vdf";

    ifstream library("C:/Program Files (x86)/Steam/steamapps/libraryfolders.vdf");
    stringstream library_buffer;
    library_buffer << library.rdbuf();

	stringstream apps_buffer;
    string line;
	bool appsLineReached = false;
    while(getline(library_buffer, line)){
       if (line.find("apps") != string::npos) {
		appsLineReached = true;
	   }
	   if (appsLineReached == true && any_of(line.begin(), line.end(), ::isdigit)) {
		line.erase(remove(line.begin(), line.end(), '\t'), line.end());

        string idLine;
        for(int i = 1; line[i] != '"'; i++) {
            idLine += line[i];
        }
        apps_buffer << idLine << endl;
	   }
       if (line.find("}") != string::npos) {
        appsLineReached = false;
        }
    }
    cout << apps_buffer.str() << endl;

    ifstream if_file(file_directory);
    stringstream config_buffer;

    line = "";
    bool appsBlockReached = false;

    while (getline(if_file, line)) {
        if(line.find("apps") != string::npos) {
            appsBlockReached = true;
            config_buffer << line << endl;
            config_buffer << "\t\t\t\t{" << endl;

            string idLine;
            while(getline(apps_buffer, idLine)) {
                config_buffer << "\t\t\t\t\t\"" << idLine << "\"" << endl;
                config_buffer << "\t\t\t\t\t{" << endl;
                config_buffer << "\t\t\t\t\t\t\"CloudEnabled\"\t\t\"0\"" << endl;
                config_buffer << "\t\t\t\t\t}" << endl;
            }
        }
        else if (appsBlockReached && line == "\t\t\t\t}") {
            appsBlockReached = false;
            config_buffer << line << endl;
        }
        else if(appsBlockReached == false){
            config_buffer << line << endl;
        }
    }
    
    ofstream outputFile(file_directory);
    outputFile << config_buffer.str();

    cout << config_buffer.str() << endl;
    return 0;
}