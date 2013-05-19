#ifndef MWMECHANICS_SECURITY_H
#define MWMECHANICS_SECURITY_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

    /// @brief implementation of Security skill
    class Security
    {
    public:
        static void pickLock (const MWWorld::Ptr& actor, const MWWorld::Ptr& lock, const MWWorld::Ptr& lockpick);
        static void probeTrap (const MWWorld::Ptr& actor, const MWWorld::Ptr& trap, const MWWorld::Ptr& probe);
    };

}

#endif
