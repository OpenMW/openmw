#include "landtexture.hpp"

#include <components/esm/esmreader.hpp>

namespace CSMWorld
{

    void LandTexture::load(ESM::ESMReader &esm)
    {
        ESM::LandTexture::load(esm);

        mPluginIndex = esm.getIndex();
    }

}
