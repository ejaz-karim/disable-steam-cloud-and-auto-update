#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/JSON/Parser.h>


#ifndef API_HPP
#define API_HPP

std::stringstream removeQuotes(const std::string &game_ids);
void apiRequest(stringstream &appIds);
std::string getJSONBody(const std::string url);
std::string getGameName(std::string jsonBody);

#endif // API_HPP