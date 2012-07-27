#include "actionalchemy.hpp"

#include "../mwbase/environment.hpp"
#include "../mwgui/window_manager.hpp"

namespace MWWorld
{
    void ActionAlchemy::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Alchemy);
    }
}
