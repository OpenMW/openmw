#include "scrollwindow.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwinput/inputmanager.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/player.hpp"
#include "../mwsound/soundmanager.hpp"

#include "formatting.hpp"
#include "window_manager.hpp"

using namespace MWGui;

ScrollWindow::ScrollWindow (WindowManager& parWindowManager) :
    WindowBase("openmw_scroll.layout", parWindowManager)
{
    getWidget(mTextView, "TextView");

    getWidget(mCloseButton, "CloseButton");
    mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onCloseButtonClicked);

    getWidget(mTakeButton, "TakeButton");
    mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onTakeButtonClicked);

    center();
}

void ScrollWindow::open (MWWorld::Ptr scroll)
{
    // no 3d sounds because the object could be in a container.
    MWBase::Environment::get().getSoundManager()->playSound ("scroll", 1.0, 1.0);

    mScroll = scroll;

    MWWorld::LiveCellRef<ESM::Book> *ref = mScroll.get<ESM::Book>();

    BookTextParser parser;
    MyGUI::IntSize size = parser.parse(ref->base->text, mTextView, 390);

    if (size.height > mTextView->getSize().height)
        mTextView->setCanvasSize(MyGUI::IntSize(410, size.height));
    else
        mTextView->setCanvasSize(410, mTextView->getSize().height);

    mTextView->setViewOffset(MyGUI::IntPoint(0,0));

    setTakeButtonShow(true);
}

void ScrollWindow::setTakeButtonShow(bool show)
{
    mTakeButton->setVisible(show);
}

void ScrollWindow::onCloseButtonClicked (MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getSoundManager()->playSound ("scroll", 1.0, 1.0);

    mWindowManager.removeGuiMode(GM_Scroll);
}

void ScrollWindow::onTakeButtonClicked (MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getSoundManager()->playSound ("Item Book Up", 1.0, 1.0, MWSound::Play_NoTrack);

    MWWorld::ActionTake take(mScroll);
    take.execute (MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

    mWindowManager.removeGuiMode(GM_Scroll);
}
