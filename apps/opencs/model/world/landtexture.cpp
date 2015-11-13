#include "landtexture.hpp"

#include <components/esm/esmreader.hpp>

namespace CSMWorld
{
    void LandTexture::load(ESM::ESMReader &esm, bool &isDeleted)
    {
        ESM::LandTexture::load(esm, isDeleted);

        mPluginIndex = esm.getIndex();
    }

}
