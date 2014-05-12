
#include "actionapply.hpp"

#include "class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{
    ActionApply::ActionApply (const Ptr& target, const std::string& id)
    : Action (false, target), mId (id)
    {}

    void ActionApply::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWorld()->breakInvisibility(actor);

        MWWorld::Class::get (getTarget()).apply (getTarget(), mId, actor);
    }


    ActionApplyWithSkill::ActionApplyWithSkill (const Ptr& target, const std::string& id,
        int skillIndex, int usageType)
    : Action (false, target), mId (id), mSkillIndex (skillIndex), mUsageType (usageType)
    {}

    void ActionApplyWithSkill::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWorld()->breakInvisibility(actor);

        if (MWWorld::Class::get (getTarget()).apply (getTarget(), mId, actor) && mUsageType!=-1)
            MWWorld::Class::get (getTarget()).skillUsageSucceeded (actor, mSkillIndex, mUsageType);
    }
}
