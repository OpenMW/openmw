#include "actionapply.hpp"

#include "class.hpp"

namespace MWWorld
{
    ActionApply::ActionApply(const Ptr& object, const ESM::RefId& id)
        : Action(false, object)
        , mId(id)
    {
    }

    void ActionApply::executeImp(const Ptr& actor)
    {
        actor.getClass().consume(getTarget(), actor);
    }
}
