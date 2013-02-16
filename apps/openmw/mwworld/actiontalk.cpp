
#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/inputmanager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : Action (false, actor) {}

    bool ActionTalk::executeImp (const Ptr& actor)
    {
        if (MWBase::Environment::get().getInputManager ()->getControlSwitch ("playercontrols"))
        {
            MWBase::Environment::get().getDialogueManager()->startDialogue (getTarget());
            return true;
        }
        return false;
    }
}
