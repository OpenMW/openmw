
#include "actionapply.hpp"

#include "class.hpp"

namespace MWWorld
{
    ActionApply::ActionApply (const Ptr& target, const std::string& id)
    : mTarget (target), mId (id)
    {}

    void ActionApply::executeImp (const Ptr& actor)
    {
        MWWorld::Class::get (mTarget).apply (mTarget, mId, actor);
    }


    ActionApplyWithSkill::ActionApplyWithSkill (const Ptr& target, const std::string& id,
        int skillIndex, int usageType)
    : mTarget (target), mId (id), mSkillIndex (skillIndex), mUsageType (usageType)
    {}

    void ActionApplyWithSkill::executeImp (const Ptr& actor)
    {
        if (MWWorld::Class::get (mTarget).apply (mTarget, mId, actor) && mUsageType!=-1)
            MWWorld::Class::get (mTarget).skillUsageSucceeded (actor, mSkillIndex, mUsageType);
    }
}
