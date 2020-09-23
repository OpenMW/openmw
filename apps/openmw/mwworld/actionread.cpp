#include "actionread.hpp"

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

        LiveCellRef<ESM::Book> *ref = getTarget().get<ESM::Book>();

        if (ref->mBase->mData.mIsScroll)
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Scroll, getTarget());
        else
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Book, getTarget());

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

            npcStats.increaseSkill (ref->mBase->mData.mSkillId, *class_, true, true);

            npcStats.flagAsUsed (ref->mBase->mId);
        }

    }
}
