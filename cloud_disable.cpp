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
    replace(input_directory.begin(), input_directory.end(), '\\', '/'); // Windows uses \ instead of / for directories which need to be switched.
    string file_directory = input_directory + "/sharedconfig.vdf"; // Creates directory to the main config file

    // Input stream class to operate on the library folders file which has all the game ids
    ifstream library("C:/Program Files (x86)/Steam/steamapps/libraryfolders.vdf");
    stringstream library_buffer; // Create a stringstream object which can be used to create a buffer to store information in
    library_buffer << library.rdbuf(); // Add the information from the library ifstream to the library buffer

	stringstream apps_buffer; // Create a stringstream object to store only the ids from the library_buffer
    string line; // String used to temporarily store each line of the library in the while loop
	bool appsLineReached = false; // Switch which tells us when the line where "apps" is found has been reached
    
    // While loop to go through each line in the library
    while(getline(library_buffer, line)){
        //if the "apps" line is found (not equal to no position), then turn the bool switch on
        if (line.find("apps") != string::npos) { 
		appsLineReached = true;
	    }

       // If the apps switch is on and the line has any digits then we enter the if statement
	    if (appsLineReached == true && any_of(line.begin(), line.end(), ::isdigit)) { 
        // Remove all the spaces in the line to give it this format -> "id""number"
        // line.erase(remove(line.begin(), line.end(), '\t'), line.end());
        remove(line.begin(), line.end(), " ");
        remove(line.begin(), line.end(), "\t");


        // Loop through each line to extract only the game id numbers. Format = "id""number" so we want to loop from i = 1 until the first " is found
        string idLine;
        for(int i = 0; line[i] != '"'; i++) {
            idLine += line[i];
        }
        apps_buffer << idLine << endl; // add the id to an apps buffer
	    }

        // Assuming we are in the apps block, if the find the } closing bracket we turn the apps switch off
        if (line.find("}") != string::npos && appsLineReached == true) {
        appsLineReached = false;
        }
    }
    // Outputs the ids to the terminal
    cout << apps_buffer.str() << endl;

    ifstream if_file(file_directory); // Input file stream created for the main config
    stringstream config_buffer; // Stringstream buffer which we will use to replace the original config

    line = "";
    bool appsBlockReached = false;

    // While loop to go through each line from the main config and add it to the config_buffer accordingly
    while (getline(if_file, line)) {
        // If apps block has been reached
        if(line.find("apps") != string::npos) {
            appsBlockReached = true;
            // Add all the lines from the original config in the same structure and format without the things within
            // the { } block. This is to ensure that we dont have repeats of information and we are only adding the id of
            // games found from the library folder we entered earlier.
            config_buffer << line << endl;
            config_buffer << "\t\t\t\t{" << endl;

            // By sticking to the format of the config, we make all ids CloudEnabled = 0 
            string idLine;
            while(getline(apps_buffer, idLine)) {
                config_buffer << "\t\t\t\t\t\"" << idLine << "\"" << endl;
                config_buffer << "\t\t\t\t\t{" << endl;
                config_buffer << "\t\t\t\t\t\t\"CloudEnabled\"\t\t\"0\"" << endl;
                config_buffer << "\t\t\t\t\t}" << endl;
            }
        }

        // If we reached the closing bracket line and we are still within the apps block, turn off the app block switch and add this line
        else if (appsBlockReached && line == "\t\t\t\t}") {
            appsBlockReached = false;
            config_buffer << line << endl;
        }

        // if we are not inside the apps block we want to add all the lines to make sure everything outside of what we are editing is added
        else if(appsBlockReached == false){
            config_buffer << line << endl;
        }
    }
    
    // Output file stream created so that we can add the new config we have created and save it replacing the old config
    ofstream outputFile(file_directory);
    outputFile << config_buffer.str();

    // Shows us the state of the config file after running this script
    cout << config_buffer.str() << endl;
    return 0;
}