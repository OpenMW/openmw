#include "actionalchemy.hpp"

#include "../mwbase/environment.hpp"
#include "../mwgui/window_manager.hpp"

namespace MWWorld
{
    void ActionAlchemy::execute()
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Alchemy);
    }
}
