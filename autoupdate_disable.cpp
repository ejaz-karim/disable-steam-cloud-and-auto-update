#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

string getDirectory(const string &prompt)
{
    string directory;
    cout << prompt;
    getline(cin, directory);
    replace(directory.begin(), directory.end(), '\\', '/');
    return directory;
}










int main(){

}