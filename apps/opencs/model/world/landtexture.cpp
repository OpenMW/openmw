#include "landtexture.hpp"

#include <sstream>
#include <stdexcept>

#include <components/esm3/esmreader.hpp>
#include <components/misc/strings/conversion.hpp>

namespace CSMWorld
{
    void LandTexture::load(ESM::ESMReader& esm, bool& isDeleted)
    {
        ESM::LandTexture::load(esm, isDeleted);

        mPluginIndex = esm.getIndex();
    }

    std::string LandTexture::createUniqueRecordId(int plugin, int index)
    {
        std::stringstream ss;
        ss << 'L' << plugin << '#' << index;
        return ss.str();
    }

    void LandTexture::parseUniqueRecordId(const std::string& id, int& plugin, int& index)
    {
        size_t middle = id.find('#');

        if (middle == std::string::npos || id[0] != 'L')
            throw std::runtime_error("Invalid LandTexture ID");

        plugin = Misc::StringUtils::toNumeric<int>(id.substr(1, middle - 1), 0);
        index = Misc::StringUtils::toNumeric<int>(id.substr(middle + 1), 0);
    }
}
