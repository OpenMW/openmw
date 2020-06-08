#ifndef OPENMW_ESM_DOORSTATE_H
#define OPENMW_ESM_DOORSTATE_H

#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct DoorState final : public ObjectState
    {
        int mDoorState = 0;

        void load (ESMReader &esm) final;
        void save (ESMWriter &esm, bool inInventory = false) const final;

        DoorState& asDoorState() final
        {
            return *this;
        }
        const DoorState& asDoorState() const final
        {
            return *this;
        }
    };
}

#endif
