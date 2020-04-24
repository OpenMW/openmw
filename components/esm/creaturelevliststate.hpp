#ifndef OPENMW_ESM_CREATURELEVLISTSTATE_H
#define OPENMW_ESM_CREATURELEVLISTSTATE_H

#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct CreatureLevListState : public ObjectState
    {
        int mSpawnActorId;
        bool mSpawn;

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm, bool inInventory = false) const;

        virtual CreatureLevListState& asCreatureLevListState()
        {
            return *this;
        }
        virtual const CreatureLevListState& asCreatureLevListState() const
        {
            return *this;
        }
    };
}

#endif
