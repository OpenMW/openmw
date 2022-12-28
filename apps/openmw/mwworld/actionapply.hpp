#ifndef GAME_MWWORLD_ACTIONAPPLY_H
#define GAME_MWWORLD_ACTIONAPPLY_H

#include "action.hpp"
#include <components/esm/refid.hpp>
#include <string>

namespace MWWorld
{
    class ActionApply : public Action
    {
        ESM::RefId mId;

        void executeImp(const Ptr& actor) override;

    public:
        ActionApply(const Ptr& object, const ESM::RefId& id);
    };

    class ActionApplyWithSkill : public Action
    {
        ESM::RefId mId;
        int mSkillIndex;
        int mUsageType;

        void executeImp(const Ptr& actor) override;

    public:
        ActionApplyWithSkill(const Ptr& object, const ESM::RefId& id, int skillIndex, int usageType);
    };
}

#endif
