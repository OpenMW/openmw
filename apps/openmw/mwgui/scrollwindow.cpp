#include "scrollwindow.hpp"

#include <components/esm/loadbook.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actiontake.hpp"

#include "formatting.hpp"

namespace
{
    void adjustButton (MWGui::ImageButton* button)
    {
        MyGUI::IntSize diff = button->getSize() - button->getRequestedSize();
        button->setSize(button->getRequestedSize());

        if (button->getAlign().isRight())
            button->setPosition(button->getPosition() + MyGUI::IntPoint(diff.width,0));
    }
}

namespace MWGui
{

    ScrollWindow::ScrollWindow ()
        : WindowBase("openmw_scroll.layout")
        , mTakeButtonShow(true)
        , mTakeButtonAllowed(true)
    {
        getWidget(mTextView, "TextView");

        getWidget(mCloseButton, "CloseButton");
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onCloseButtonClicked);

        getWidget(mTakeButton, "TakeButton");
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ScrollWindow::onTakeButtonClicked);

        adjustButton(mCloseButton);
        adjustButton(mTakeButton);

        center();
    }

    void ScrollWindow::open (MWWorld::Ptr scroll)
    {
        // no 3d sounds because the object could be in a container.
        MWBase::Environment::get().getSoundManager()->playSound ("scroll", 1.0, 1.0);

        mScroll = scroll;

        MWWorld::LiveCellRef<ESM::Book> *ref = mScroll.get<ESM::Book>();

        BookTextParser parser;
        MyGUI::IntSize size = parser.parseScroll(ref->mBase->mText, mTextView, 390);

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mTextView->setVisibleVScroll(false);
        if (size.height > mTextView->getSize().height)
            mTextView->setCanvasSize(MyGUI::IntSize(410, size.height));
        else
            mTextView->setCanvasSize(410, mTextView->getSize().height);
        mTextView->setVisibleVScroll(true);

        mTextView->setViewOffset(MyGUI::IntPoint(0,0));

        setTakeButtonShow(true);
    }

    void ScrollWindow::exit()
    {
        MWBase::Environment::get().getSoundManager()->playSound ("scroll", 1.0, 1.0);

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll);
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
        exit();
    }

    void ScrollWindow::onTakeButtonClicked (MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getSoundManager()->playSound("Item Book Up", 1.0, 1.0);

        MWWorld::ActionTake take(mScroll);
        take.execute (MWBase::Environment::get().getWorld()->getPlayerPtr());

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll);
    }
}
