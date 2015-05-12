#ifndef GAME_MWWORLD_CELLFUNCTORS_H
#define GAME_MWWORLD_CELLFUNCTORS_H

#include <vector>
#include <string>

#include "ptr.hpp"


namespace MWWorld
{
    struct ListAndResetObjects
    {
        std::vector<MWWorld::Ptr> mObjects;

        bool operator() (MWWorld::Ptr ptr)
        {
            if (ptr.getRefData().getBaseNode())
            {
                ptr.getRefData().setBaseNode(NULL);
                mObjects.push_back (ptr);
            }

            return true;
        }
    };
}

#endif
