#include "land.hpp"

#include <sstream>
#include <stdexcept>

#include <components/misc/strings/conversion.hpp>

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    void Land::load(ESM::ESMReader& esm, bool& isDeleted)
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

        x = Misc::StringUtils::toNumeric<int>(id.substr(1, mid - 1), 0);
        y = Misc::StringUtils::toNumeric<int>(id.substr(mid + 1), 0);
    }
}
