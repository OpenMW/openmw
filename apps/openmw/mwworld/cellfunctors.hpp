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
        std::vector<Ogre::SceneNode*> mHandles;

        bool operator() (ESM::CellRef& ref, RefData& data)
        {
            Ogre::SceneNode* handle = data.getBaseNode();
            if (handle)
                mHandles.push_back (handle);
            return true;
        }
    };
}

#endif
