
#include "actionapply.hpp"

#include "class.hpp"

namespace MWWorld
{
    ActionApply::ActionApply (const Ptr& target, const std::string& id, const Ptr& actor)
    : mTarget (target), mId (id), mActor (actor)
    {}

    void ActionApply::execute()
    {
        MWWorld::Class::get (mTarget).apply (mTarget, mId, mActor);
    }


    ActionApplyWithSkill::ActionApplyWithSkill (const Ptr& target, const std::string& id,
        const Ptr& actor, int skillIndex, int usageType)
    : mTarget (target), mId (id), mActor (actor), mSkillIndex (skillIndex), mUsageType (usageType)
    {}

    void ActionApplyWithSkill::execute()
    {
        if (MWWorld::Class::get (mTarget).apply (mTarget, mId, mActor))
            MWWorld::Class::get (mTarget).skillUsageSucceeded (mActor, mSkillIndex, mUsageType);
    }
}
