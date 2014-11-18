#include "landtexture.hpp"

#include <components/esm/esmreader.hpp>

namespace CSMWorld
{

    void LandTexture::load(ESM::ESMReader &esm)
    {
        ESM::LandTexture::load(esm);

        int plugin = esm.getIndex();

        std::ostringstream stream;

        stream << mIndex << "_" << plugin;

        mId = stream.str();
    }

}
