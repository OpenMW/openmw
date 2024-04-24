#include "actionread.hpp"

#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadskil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "class.hpp"
#include "esmstore.hpp"

namespace MWWorld
{
    ActionRead::ActionRead(const MWWorld::Ptr& object)
        : Action(false, object)
    {
    }

    void ActionRead::executeImp(const MWWorld::Ptr& actor)
    {

        if (actor != MWMechanics::getPlayer())
            return;

        // Ensure we're not in combat
        if (MWMechanics::isPlayerInCombat()
            // Reading in combat is still allowed if the scroll/book is not in the player inventory yet
            // (since otherwise, there would be no way to pick it up)
            && getTarget().getContainerStore() == &actor.getClass().getContainerStore(actor))
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage4}");
            return;
        }

        LiveCellRef<ESM::Book>* ref = getTarget().get<ESM::Book>();

        if (ref->mBase->mData.mIsScroll)
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Scroll, getTarget());
        else
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Book, getTarget());

        MWMechanics::NpcStats& npcStats = actor.getClass().getNpcStats(actor);

        // Skill gain from books
        ESM::RefId skill = ESM::Skill::indexToRefId(ref->mBase->mData.mSkillId);
        if (!skill.empty() && !npcStats.hasBeenUsed(ref->mBase->mId))
        {
            MWBase::Environment::get().getLuaManager()->skillLevelUp(actor, skill, "book");

            npcStats.flagAsUsed(ref->mBase->mId);
        }
    }
}
