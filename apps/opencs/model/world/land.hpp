#ifndef CSM_WORLD_LAND_H
#define CSM_WORLD_LAND_H

#include <string>
#include <memory>

#include <components/esm/loadland.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for Land record. Encodes X and Y cell index in the ID.
    ///
    /// \todo Add worldspace support to the Land record.
    /// \todo Add a proper copy constructor (currently worked around using shared_ptr)
    struct Land
    {
        Land();

        std::shared_ptr<ESM::Land> mLand;

        std::string mId;

        /// Loads the metadata and ID
        void load (ESM::ESMReader &esm);

        void blank();
    };
}

#endif
