#include "actionalchemy.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWWorld
{
    bool ActionAlchemy::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Alchemy);
        return true;
    }
}
