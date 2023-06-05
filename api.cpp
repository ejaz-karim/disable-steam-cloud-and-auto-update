#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>

using namespace std;

// Function that takes a string of gameIds and sends each line to the api
string url = "https://store.steampowered.com/api/appdetails?appids=";

stringstream removeQuotes(const string &game_ids){
    stringstream ss(game_ids);
    string line;
    stringstream noQuotes;
    while(getline(ss, line)){
        line.erase(line.find_first_of('"'), 1);
        line.erase(line.find_last_of('"'), 1);
        noQuotes << line << endl;
    }
    return noQuotes;


}

void apiRequest(stringstream &appIds){
    string line;
    while(getline(appIds, line)){
        string temp = "";
        temp = url + line;
        cout << temp << endl;
    }



}