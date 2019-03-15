#ifndef GAME_MWWORLD_ACTIONAPPLY_H
#define GAME_MWWORLD_ACTIONAPPLY_H

#include <string>

#include "action.hpp"

namespace MWWorld
{
    class ActionApply : public Action
    {
            std::string mId;

            virtual void executeImp (const Ptr& actor);

        public:

            ActionApply (const Ptr& object, const std::string& id);
    };

    class ActionApplyWithSkill : public Action
    {
            std::string mId;
            int mSkillIndex;
            int mUsageType;

            virtual void executeImp (const Ptr& actor);

        public:

            ActionApplyWithSkill (const Ptr& object, const std::string& id,
                int skillIndex, int usageType);
    };
}

#endif
