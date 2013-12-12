
#include "actiontalk.hpp"

#include "class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwmechanics/creaturestats.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk (const Ptr& actor) : Action (false, actor) {}

    void ActionTalk::executeImp (const Ptr& actor)
    {
        MWWorld::Ptr talkTo = getTarget();	//because 'actor' is always the player!
        if ( MWWorld::Class::get(talkTo).getCreatureStats(talkTo).isHostile() )
            return;

        MWBase::Environment::get().getDialogueManager()->startDialogue (talkTo);
    }
}
