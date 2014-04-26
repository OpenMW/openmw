#include "actionalchemy.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

namespace MWWorld
{
    void ActionAlchemy::executeImp (const Ptr& actor)
    {
        if(MWBase::Environment::get().getWorld()->getPlayer().isInCombat()) { //Ensure we're not in combat
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage3}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Alchemy);
    }
}
