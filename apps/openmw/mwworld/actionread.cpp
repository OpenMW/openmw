#include "actionread.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwgui/bookwindow.hpp"
#include "../mwgui/scrollwindow.hpp"

#include "player.hpp"
#include "class.hpp"
#include "esmstore.hpp"

namespace MWWorld
{
    ActionRead::ActionRead (const MWWorld::Ptr& object) : Action (false, object)
    {
    }

    void ActionRead::executeImp (const MWWorld::Ptr& actor) {

        if(MWBase::Environment::get().getWorld()->getPlayer().isInCombat()) { //Ensure we're not in combat
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage4}");
            return;
        }

        LiveCellRef<ESM::Book> *ref = getTarget().get<ESM::Book>();

        if (ref->mBase->mData.mIsScroll)
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Scroll);
            MWBase::Environment::get().getWindowManager()->getScrollWindow()->open(getTarget());
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Book);
            MWBase::Environment::get().getWindowManager()->getBookWindow()->open(getTarget());
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get(player).getNpcStats (player);

        // Skill gain from books
        if (ref->mBase->mData.mSkillID >= 0 && ref->mBase->mData.mSkillID < ESM::Skill::Length
                && !npcStats.hasBeenUsed (ref->mBase->mId))
        {
            MWWorld::LiveCellRef<ESM::NPC> *playerRef = player.get<ESM::NPC>();

            const ESM::Class *class_ =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (
                    playerRef->mBase->mClass
                );

            npcStats.increaseSkill (ref->mBase->mData.mSkillID, *class_, true);

            npcStats.flagAsUsed (ref->mBase->mId);
        }

    }
}
