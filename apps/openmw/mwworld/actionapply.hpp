
#ifndef GAME_MWWORLD_ACTIONAPPLY_H
#define GAME_MWWORLD_ACTIONAPPLY_H

#include <string>

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionApply : public Action
    {
            Ptr mTarget;
            std::string mId;

            virtual void executeImp (const Ptr& actor);

        public:

            ActionApply (const Ptr& target, const std::string& id);
    };

    class ActionApplyWithSkill : public Action
    {
            Ptr mTarget;
            std::string mId;
            int mSkillIndex;
            int mUsageType;

            virtual void executeImp (const Ptr& actor);

        public:

            ActionApplyWithSkill (const Ptr& target, const std::string& id,
                int skillIndex, int usageType);
    };
}

#endif
