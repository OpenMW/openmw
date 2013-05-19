#ifndef MWMECHANICS_SECURITY_H
#define MWMECHANICS_SECURITY_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

    /// @brief implementation of Security skill
    class Security
    {
    public:
        static void pickLock (const MWWorld::Ptr& actor, const MWWorld::Ptr& lock, const MWWorld::Ptr& lockpick,
                              std::string& resultMessage, std::string& resultSound);
        static void probeTrap (const MWWorld::Ptr& actor, const MWWorld::Ptr& trap, const MWWorld::Ptr& probe,
                               std::string& resultMessage, std::string& resultSound);
    };

}

#endif
