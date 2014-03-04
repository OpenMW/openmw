#ifndef OPENMW_ESM_LIGHTSTATE_H
#define OPENMW_ESM_LIGHTSTATE_H

#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct LightState : public ObjectState
    {
        float mTime;

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm, bool inInventory = false) const;
    };
}

#endif
