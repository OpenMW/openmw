
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
            Ptr mActor;

        public:

            ActionApply (const Ptr& target, const std::string& id, const Ptr& actor);

            virtual void execute();
    };

    class ActionApplyWithSkill : public Action
    {
            Ptr mTarget;
            std::string mId;
            Ptr mActor;
            int mSkillIndex;
            int mUsageType;

        public:

            ActionApplyWithSkill (const Ptr& target, const std::string& id, const Ptr& actor,
                int skillIndex, int usageType);

            virtual void execute();
    };
}

#endif
