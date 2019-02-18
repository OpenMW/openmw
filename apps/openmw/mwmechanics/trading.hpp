#ifndef OPENMW_MECHANICS_TRADING_H
#define OPENMW_MECHANICS_TRADING_H

namespace MWWorld
{
    class Ptr;
}

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
