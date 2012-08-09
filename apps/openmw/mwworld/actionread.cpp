#include "actionread.hpp"

#include "../mwbase/environment.hpp"
#include "../mwgui/window_manager.hpp"
#include "../mwgui/bookwindow.hpp"
#include "../mwgui/scrollwindow.hpp"

namespace MWWorld
{
    ActionRead::ActionRead (const MWWorld::Ptr& object) : mObject (object)
    {
    }

    void ActionRead::executeImp (const MWWorld::Ptr& actor)
    {
        LiveCellRef<ESM::Book> *ref = mObject.get<ESM::Book>();

        if (ref->base->data.isScroll)
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Scroll);
            MWBase::Environment::get().getWindowManager()->getScrollWindow()->open(mObject);
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Book);
            MWBase::Environment::get().getWindowManager()->getBookWindow()->open(mObject);
        }
    }
}
