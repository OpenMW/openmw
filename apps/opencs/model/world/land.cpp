#include "land.hpp"

#include <sstream>
#include <stdexcept>

namespace CSMWorld
{
    void Land::load(ESM::ESMReader &esm, bool &isDeleted)
    {
        ESM::Land::load(esm, isDeleted);
    }

    std::string Land::createUniqueRecordId(int x, int y)
    {
        std::ostringstream stream;
        stream << "#" << x << " " << y;
        return stream.str();
    }

    void Land::parseUniqueRecordId(const std::string& id, int& x, int& y)
    {
        size_t mid = id.find(' ');

        if (mid == std::string::npos || id[0] != '#')
            throw std::runtime_error("Invalid Land ID");

        x = std::stoi(id.substr(1, mid - 1));
        y = std::stoi(id.substr(mid + 1));
    }
}
