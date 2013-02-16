
#include "actionapply.hpp"

#include "class.hpp"

namespace MWWorld
{
    ActionApply::ActionApply (const Ptr& target, const std::string& id)
    : Action (false, target), mId (id)
    {}

    bool ActionApply::executeImp (const Ptr& actor)
    {
        MWWorld::Class::get (getTarget()).apply (getTarget(), mId, actor);
        return true;
    }


    ActionApplyWithSkill::ActionApplyWithSkill (const Ptr& target, const std::string& id,
        int skillIndex, int usageType)
    : Action (false, target), mId (id), mSkillIndex (skillIndex), mUsageType (usageType)
    {}

    bool ActionApplyWithSkill::executeImp (const Ptr& actor)
    {
        if (MWWorld::Class::get (getTarget()).apply (getTarget(), mId, actor) && mUsageType!=-1)
            MWWorld::Class::get (getTarget()).skillUsageSucceeded (actor, mSkillIndex, mUsageType);
        return true;
    }
}
