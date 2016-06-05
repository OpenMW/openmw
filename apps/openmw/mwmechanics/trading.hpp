#ifndef OPENMW_MECHANICS_TRADING_H
#define OPENMW_MECHANICS_TRADING_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    class Trading
    {
    public:
        Trading();

        bool haggle(const MWWorld::Ptr& player, const MWWorld::Ptr& merchant, int playerOffer, int merchantOffer);
    };
}

#endif
