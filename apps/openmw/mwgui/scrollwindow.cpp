#include "scrollwindow.hpp"

#include <MyGUI_ScrollView.h>

#include <components/esm/loadbook.hpp>
#include <components/widgets/imagebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/actiontake.hpp"
#include "../mwworld/class.hpp"

#include "formatting.hpp"

namespace MWGui
{

    ScrollWindow::ScrollWindow ()
        : BookWindowBase("openmw_scroll.layout")
        , mTakeButtonShow(true)
        , mTakeButtonAllowed(true)
    {
        getWidget(mTextView, "TextView");

        getWidget(mCloseButton, "CloseButton");
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onCloseButtonClicked);

        getWidget(mTakeButton, "TakeButton");
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onTakeButtonClicked);

        adjustButton("CloseButton");
        adjustButton("TakeButton");

        mCloseButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &ScrollWindow::onKeyButtonPressed);
        mTakeButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &ScrollWindow::onKeyButtonPressed);

        center();
    }

    void ScrollWindow::setPtr (const MWWorld::Ptr& scroll)
    {
        mScroll = scroll;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        bool showTakeButton = scroll.getContainerStore() != &player.getClass().getContainerStore(player);

        MWWorld::LiveCellRef<ESM::Book> *ref = mScroll.get<ESM::Book>();

        Formatting::BookFormatter formatter;
        formatter.markupToWidget(mTextView, ref->mBase->mText, 390, mTextView->getHeight());
        MyGUI::IntSize size = mTextView->getChildAt(0)->getSize();

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mTextView->setVisibleVScroll(false);
        if (size.height > mTextView->getSize().height)
            mTextView->setCanvasSize(mTextView->getWidth(), size.height);
        else
            mTextView->setCanvasSize(mTextView->getWidth(), mTextView->getSize().height);
        mTextView->setVisibleVScroll(true);

        mTextView->setViewOffset(MyGUI::IntPoint(0,0));

        setTakeButtonShow(showTakeButton);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);
    }

    void ScrollWindow::onKeyButtonPressed(MyGUI::Widget *sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        int scroll = 0;
        if (key == MyGUI::KeyCode::ArrowUp)
            scroll = 40;
        else if (key == MyGUI::KeyCode::ArrowDown)
            scroll = -40;

        if (scroll != 0)
            mTextView->setViewOffset(mTextView->getViewOffset() + MyGUI::IntPoint(0, scroll));
    }

    void ScrollWindow::setTakeButtonShow(bool show)
    {
        mTakeButtonShow = show;
        mTakeButton->setVisible(mTakeButtonShow && mTakeButtonAllowed);
    }

    void ScrollWindow::setInventoryAllowed(bool allowed)
    {
        mTakeButtonAllowed = allowed;
        mTakeButton->setVisible(mTakeButtonShow && mTakeButtonAllowed);
    }

    void ScrollWindow::onCloseButtonClicked (MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll);
    }

    void ScrollWindow::onTakeButtonClicked (MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->playSound("Item Book Up");

        MWWorld::ActionTake take(mScroll);
        take.execute (MWMechanics::getPlayer());

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll, true);
    }
}
