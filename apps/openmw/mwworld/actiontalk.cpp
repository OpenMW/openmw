
#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"

#include "../mwdialogue/dialoguemanager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : mActor (actor) {}

    void ActionTalk::execute()
    {
        MWBase::Environment::get().getDialogueManager()->startDialogue (mActor);
    }
}
