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
        actor.getClass().consume(getTarget(), actor);
    }


    ActionApplyWithSkill::ActionApplyWithSkill (const Ptr& object, const std::string& id,
        int skillIndex, int usageType)
    : Action (false, object), mId (id), mSkillIndex (skillIndex), mUsageType (usageType)
    {}

    void ActionApplyWithSkill::executeImp (const Ptr& actor)
    {
        bool consumed = actor.getClass().consume(getTarget(), actor);
        if (consumed && mUsageType != -1 && actor == MWMechanics::getPlayer())
            actor.getClass().skillUsageSucceeded (actor, mSkillIndex, mUsageType);
    }
}
