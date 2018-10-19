#ifndef GAME_MWWORLD_CELLVISITORS_H
#define GAME_MWWORLD_CELLVISITORS_H

#include <vector>
#include <string>

#include "ptr.hpp"


namespace MWWorld
{
    struct ListAndResetObjectsVisitor
    {
        std::vector<MWWorld::Ptr> mObjects;

        bool operator() (MWWorld::Ptr ptr)
        {
            if (ptr.getRefData().getBaseNode())
            {
                ptr.getRefData().setBaseNode(nullptr);
                mObjects.push_back (ptr);
            }

            return true;
        }
    };
}

#endif
