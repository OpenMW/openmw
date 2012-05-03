#include "scrollwindow.hpp"

#include "../mwbase/environment.hpp"
#include "../mwinput/inputmanager.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwsound/soundmanager.hpp"

using namespace MWGui;

ScrollWindow::ScrollWindow (WindowManager& parWindowManager) :
    WindowBase("openmw_scroll_layout.xml", parWindowManager)
{
    getWidget(mCloseButton, "CloseButton");
    mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onCloseButtonClicked);

    getWidget(mTakeButton, "TakeButton");
    mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onTakeButtonClicked);

    center();
}

void ScrollWindow::open (MWWorld::Ptr scroll)
{
    MWBase::Environment::get().getSoundManager()->playSound3D (scroll, "scroll", 1.0, 1.0);

    mScroll = scroll;
}

void ScrollWindow::onCloseButtonClicked (MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getSoundManager()->playSound3D (mScroll, "scroll", 1.0, 1.0);

    MWBase::Environment::get().getInputManager()->setGuiMode (GM_Game);
}

void ScrollWindow::onTakeButtonClicked (MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getSoundManager()->playSound3D (mScroll, "Item Book Up", 1.0, 1.0, MWSound::Play_NoTrack);

    MWWorld::ActionTake take(mScroll);
    take.execute();

    MWBase::Environment::get().getInputManager()->setGuiMode (GM_Game);
}
