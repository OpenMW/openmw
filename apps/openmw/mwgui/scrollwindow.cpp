#include "scrollwindow.hpp"

#include <MyGUI_ScrollView.h>

#include <components/esm/loadbook.hpp>
#include <components/widgets/imagebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/actiontake.hpp"

#include "formatting.hpp"

namespace
{
    void adjustButton (Gui::ImageButton* button)
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

    void ScrollWindow::openScroll (MWWorld::Ptr scroll, bool showTakeButton)
    {
        // no 3d sounds because the object could be in a container.
        MWBase::Environment::get().getSoundManager()->playSound ("scroll", 1.0, 1.0);

        mScroll = scroll;

        MWWorld::LiveCellRef<ESM::Book> *ref = mScroll.get<ESM::Book>();

        Formatting::BookFormatter formatter;
        formatter.markupToWidget(mTextView, ref->mBase->mText, 390, mTextView->getHeight());
        MyGUI::IntSize size = mTextView->getChildAt(0)->getSize();

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mTextView->setVisibleVScroll(false);
        if (size.height > mTextView->getSize().height)
            mTextView->setCanvasSize(MyGUI::IntSize(410, size.height));
        else
            mTextView->setCanvasSize(410, mTextView->getSize().height);
        mTextView->setVisibleVScroll(true);

        mTextView->setViewOffset(MyGUI::IntPoint(0,0));

        setTakeButtonShow(showTakeButton);
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
        take.execute (MWMechanics::getPlayer());

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Scroll);
    }
}
