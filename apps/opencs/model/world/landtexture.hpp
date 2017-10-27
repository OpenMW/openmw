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

        /// Returns a string identifier that will be unique to any LandTexture.
        static std::string createUniqueRecordId(int plugin, int index);
        /// Deconstructs a unique string identifier into plugin and index.
        static void parseUniqueRecordId(const std::string& id, int& plugin, int& index);
    };
}

#endif
