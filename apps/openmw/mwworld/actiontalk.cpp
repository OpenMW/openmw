#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : Action (false, actor) {}

    void ActionTalk::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue, getTarget());
    }
}
