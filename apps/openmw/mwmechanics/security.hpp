#ifndef MWMECHANICS_SECURITY_H
#define MWMECHANICS_SECURITY_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

    /// @brief implementation of Security skill
    class Security
    {
    public:
        Security(const MWWorld::Ptr& actor);

        void pickLock(const MWWorld::Ptr& lock, const MWWorld::Ptr& lockpick, std::string_view& resultMessage,
            std::string_view& resultSound);
        void probeTrap(const MWWorld::Ptr& trap, const MWWorld::Ptr& probe, std::string_view& resultMessage,
            std::string_view& resultSound);

    private:
        float mAgility, mLuck, mSecuritySkill, mFatigueTerm;
        MWWorld::Ptr mActor;
    };

}

#endif
