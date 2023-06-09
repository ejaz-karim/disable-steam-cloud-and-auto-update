#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>
#include "../include/steam-cloud-and-autoupdate-disable/api.hpp"
// #include <Poco/Net/HTTPClientSession.h>
// #include <Poco/Net/HTTPRequest.h>
// #include <Poco/Net/HTTPResponse.h>
// #include <Poco/StreamCopier.h>
// #include <Poco/URI.h>
// #include <Poco/JSON/Parser.h>

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

// string getJSONBody(const string url) {
//     Creating the client session
//     Poco::URI uri(url);
//     Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

//     // Create the request
//     Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET uri.getpath(), Poco::Net::HTTPRequest::HTTP_1_1);
//     session.sendRequest(request);

//     // Get the response
//     Poco::Net::HTTPResponse response;
//     std::istream& responseBody = session.receiveResponse(response);

//     // Copy the response into a string
//     std::string jsonBody;
//     Poco::StreamCopier::copyToString(responseBody, jsonBody);
//     return jsonBody;
// }

// string getGameName(string jsonBody) {
    
// }

// void apiRequest(stringstream &appIds){
//     string line;
//     while(getline(appIds, line)){
//         string temp = "";
//         temp = url + line;
//         cout << getJSONBody(temp) << endl;
//     }
// }