#include "actionread.hpp"

/*
    Start of tes3mp addition

    Include additional headers for multiplayer purposes
*/
#include "../mwmp/Main.hpp"
#include "../mwmp/LocalPlayer.hpp"
/*
    End of tes3mp addition
*/

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "player.hpp"
#include "class.hpp"
#include "esmstore.hpp"

namespace MWWorld
{
    ActionRead::ActionRead (const MWWorld::Ptr& object) : Action (false, object)
    {
    }

    void ActionRead::executeImp (const MWWorld::Ptr& actor) {

        if (actor != MWMechanics::getPlayer())
            return;

        //Ensure we're not in combat
        if(MWMechanics::isPlayerInCombat()
                // Reading in combat is still allowed if the scroll/book is not in the player inventory yet
                // (since otherwise, there would be no way to pick it up)
                && getTarget().getContainerStore() == &actor.getClass().getContainerStore(actor)
                ) {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage4}");
            return;
        }

        bool showTakeButton = (getTarget().getContainerStore() != &actor.getClass().getContainerStore(actor));

        LiveCellRef<ESM::Book> *ref = getTarget().get<ESM::Book>();

        if (ref->mBase->mData.mIsScroll)
            MWBase::Environment::get().getWindowManager()->showScroll(getTarget(), showTakeButton);
        else
            MWBase::Environment::get().getWindowManager()->showBook(getTarget(), showTakeButton);

        MWMechanics::NpcStats& npcStats = actor.getClass().getNpcStats (actor);

        // Skill gain from books
        if (ref->mBase->mData.mSkillId >= 0 && ref->mBase->mData.mSkillId < ESM::Skill::Length
                && !npcStats.hasBeenUsed (ref->mBase->mId))
        {
            MWWorld::LiveCellRef<ESM::NPC> *playerRef = actor.get<ESM::NPC>();

            const ESM::Class *class_ =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (
                    playerRef->mBase->mClass
                );

            npcStats.increaseSkill (ref->mBase->mData.mSkillId, *class_, true);

            npcStats.flagAsUsed (ref->mBase->mId);

            /*
                Start of tes3mp addition

                Send an ID_PLAYER_BOOK packet every time a player reads a skill book
            */
            mwmp::Main::get().getLocalPlayer()->sendBook(ref->mBase->mId);
            /*
                End of tes3mp addition
            */
        }

    }
}
