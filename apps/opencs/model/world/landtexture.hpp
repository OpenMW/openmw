#ifndef CSM_WORLD_LANDTEXTURE_H
#define CSM_WORLD_LANDTEXTURE_H

#include <string>

#include <components/esm/loadltex.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for LandTexture record, providing info which plugin the LandTexture was loaded from.
    struct LandTexture : public ESM::LandTexture
    {
        int mPluginIndex;

        void load (ESM::ESMReader &esm, bool &isDeleted);
    };
}

#endif
