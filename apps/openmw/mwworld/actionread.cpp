#include "actionread.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwgui/bookwindow.hpp"
#include "../mwgui/scrollwindow.hpp"

#include <components/esm_store/store.hpp>

namespace MWWorld
{
    ActionRead::ActionRead (const MWWorld::Ptr& object) : Action (false, object)
    {
    }

    void ActionRead::executeImp (const MWWorld::Ptr& actor)
    {
        LiveCellRef<ESM::Book> *ref = getTarget().get<ESM::Book>();

        if (ref->base->mData.mIsScroll)
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Scroll);
            MWBase::Environment::get().getWindowManager()->getScrollWindow()->open(getTarget());
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Book);
            MWBase::Environment::get().getWindowManager()->getBookWindow()->open(getTarget());
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer().getPlayer();
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get(player).getNpcStats (player);

        // Skill gain from books
        if (ref->base->mData.mSkillID >= 0 && ref->base->mData.mSkillID < ESM::Skill::Length
                && !npcStats.hasBeenUsed (ref->base->mId))
        {
            MWWorld::LiveCellRef<ESM::NPC> *playerRef = player.get<ESM::NPC>();
            const ESM::Class *class_ = MWBase::Environment::get().getWorld()->getStore().classes.find (
                playerRef->base->mClass);

            npcStats.increaseSkill (ref->base->mData.mSkillID, *class_, true);

            npcStats.flagAsUsed (ref->base->mId);
        }

    }
}
