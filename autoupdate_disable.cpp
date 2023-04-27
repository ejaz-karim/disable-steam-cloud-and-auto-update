#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

int main() {
    string input_directory;

    // C:\Program Files (x86)\Steam\userdata\<id>\7\remote is the directory in Windows
    cout << "Enter directory: ";
    getline(cin, input_directory);
    replace(input_directory.begin(), input_directory.end(), '\\', '/');

    // Get a list of all files in the directory
    for (auto& file : fs::directory_iterator(input_directory)) {
        // Check if the file extension is ".acf"
        if (file.path().extension() == ".acf") {
            string file_directory = file.path().string();

            // Open the file and read its contents
            ifstream if_file(file_directory);
            stringstream buffer;
            buffer << if_file.rdbuf();
            string contents = buffer.str();

            // Apply the regex_replace function
            regex pattern("\"AutoUpdateBehavior\"\t\t\"0\"");
            string replacement("\"AutoUpdateBehavior\"\t\t\"1\"");
            string result = regex_replace(contents, pattern, replacement);

            // Write the modified contents back to the same file
            if_file.close();
            ofstream of_file(file_directory);
            of_file << result;
            of_file.close();
        }
    }

    return 0;
}
