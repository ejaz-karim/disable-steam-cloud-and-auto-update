#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>

class Api
{
public:
    Api();
    std::stringstream removeQuotes(const std::string &game_ids);
};
