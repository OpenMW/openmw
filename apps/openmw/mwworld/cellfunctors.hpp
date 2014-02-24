#ifndef GAME_MWWORLD_CELLFUNCTORS_H
#define GAME_MWWORLD_CELLFUNCTORS_H

#include <vector>
#include <string>

#include "ptr.hpp"

namespace ESM
{
    class CellRef;
}

namespace MWWorld
{
    /// List all (Ogre-)handles, then reset RefData::mBaseNode to 0.
    struct ListAndResetHandles
    {
        std::vector<Ogre::SceneNode*> mHandles;

        bool operator() (MWWorld::Ptr ptr)
        {
            Ogre::SceneNode* handle = ptr.getRefData().getBaseNode();
            if (handle)
                mHandles.push_back (handle);

            ptr.getRefData().setBaseNode(0);
            return true;
        }
    };
}

#endif
