#ifndef OPENMW_ESM_DOORSTATE_H
#define OPENMW_ESM_DOORSTATE_H

#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct DoorState : public ObjectState
    {
        int mDoorState;

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm, bool inInventory = false) const;
    };
}

#endif
