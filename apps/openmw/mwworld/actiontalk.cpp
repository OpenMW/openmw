#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : Action (false, actor) {}

    void ActionTalk::executeImp (const Ptr& actor)
    {
        if (actor == MWMechanics::getPlayer())
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue, getTarget());
    }
}
