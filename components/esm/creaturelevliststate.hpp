#ifndef OPENMW_ESM_CREATURELEVLISTSTATE_H
#define OPENMW_ESM_CREATURELEVLISTSTATE_H

#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct CreatureLevListState final : public ObjectState
    {
        int mSpawnActorId;
        bool mSpawn;

        void load (ESMReader &esm) final;
        void save (ESMWriter &esm, bool inInventory = false) const final;

        CreatureLevListState& asCreatureLevListState() final
        {
            return *this;
        }
        const CreatureLevListState& asCreatureLevListState() const final
        {
            return *this;
        }
    };
}

#endif
