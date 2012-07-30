
#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"
#include "../mwgui/window_manager.hpp"
#include "../mwdialogue/dialoguemanager.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : mActor (actor) {}

    void ActionTalk::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getDialogueManager()->startDialogue (mActor);
    }
}
