#ifndef OPENMW_MWMECHANICS_CREATURECUSTOMDATARESETTER_H
#define OPENMW_MWMECHANICS_CREATURECUSTOMDATARESETTER_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    struct CreatureCustomDataResetter
    {
        MWWorld::Ptr mPtr;

        ~CreatureCustomDataResetter()
        {
            if (!mPtr.isEmpty())
                mPtr.getRefData().setCustomData({});
        }
    };
}

#endif
