#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
    // Open the test.vdf file for reading
    ifstream inputFile("test.vdf");

    // Create a string to hold the modified file content
    string modifiedContent;

    // Loop through each line of the input file
    string line;
    while (getline(inputFile, line)) {
        // Check if the line contains "CloudEnabled" "1"
        size_t found = line.find("\"CloudEnabled\"\t\t\"1\"");
        if (found != string::npos) {
            // Replace "CloudEnabled" "1" with "CloudEnabled" "0"
            line.replace(found, 22, "\"CloudEnabled\"\t\t\"0\"");
        }

        // Add the line to the modified content string
        modifiedContent += line + "\n";
    }

    // Close the input file
    inputFile.close();

    // Open the test.vdf file for writing
    ofstream outputFile("test.vdf");

    // Write the modified content to the output file
    outputFile << modifiedContent;

    // Close the output file
    outputFile.close();

    return 0;
}
