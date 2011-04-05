#ifndef GAME_MWWORLD_CELLFUNCTORS_H
#define GAME_MWWORLD_CELLFUNCTORS_H

#include <vector>
#include <string>

#include "refdata.hpp"

namespace ESM
{
    class CellRef;
}

namespace MWWorld
{
    /// List all (Ogre-)handles.
    struct ListHandles
    {
        std::vector<std::string> mHandles;

        bool operator() (ESM::CellRef& ref, RefData& data)
        {
            std::string handle = data.getHandle();
            if (!handle.empty())
                mHandles.push_back (handle);
            return true;
        }
    };
}

#endif
