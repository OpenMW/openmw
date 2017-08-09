#include "actionapply.hpp"

#include "class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace MWWorld
{
    ActionApply::ActionApply (const Ptr& object, const std::string& id)
    : Action (false, object), mId (id)
    {}

    void ActionApply::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWorld()->breakInvisibility(actor);

        actor.getClass().apply (actor, mId, actor);

        actor.getClass().getContainerStore(actor).remove(getTarget(), 1, actor);
    }


    ActionApplyWithSkill::ActionApplyWithSkill (const Ptr& object, const std::string& id,
        int skillIndex, int usageType)
    : Action (false, object), mId (id), mSkillIndex (skillIndex), mUsageType (usageType)
    {}

    void ActionApplyWithSkill::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWorld()->breakInvisibility(actor);

        if (actor.getClass().apply (actor, mId, actor) && mUsageType!=-1 && actor == MWMechanics::getPlayer())
            actor.getClass().skillUsageSucceeded (actor, mSkillIndex, mUsageType);

        actor.getClass().getContainerStore(actor).remove(getTarget(), 1, actor);
    }
}
