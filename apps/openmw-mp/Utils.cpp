//
// Created by koncord on 04.03.17.
//

#include "Utils.hpp"

using namespace std;

const vector<string> Utils::split(const string &str, int delimiter)
{
    string buffer;
    vector<string> result;

    for (auto symb:str)
        if (symb != delimiter)
            buffer += symb;
        else if (!buffer.empty())
        {
            result.push_back(move(buffer));
            buffer.clear();
        }
    if (!buffer.empty())
        result.push_back(move(buffer));

    return result;
}