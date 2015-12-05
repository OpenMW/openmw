#ifndef CSM_WORLD_LAND_H
#define CSM_WORLD_LAND_H

#include <string>

#include <components/esm/loadland.hpp>

namespace CSMWorld
{
    /// \brief Wrapper for Land record. Encodes X and Y cell index in the ID.
    ///
    /// \todo Add worldspace support to the Land record.
    struct Land : public ESM::Land
    {
        std::string mId;

        /// Loads the metadata and ID
        void load (ESM::ESMReader &esm, bool &isDeleted);
    };
}

#endif
