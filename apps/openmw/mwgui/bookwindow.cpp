#include "bookwindow.hpp"

#include <MyGUI_TextBox.h>

#include <components/esm/loadbook.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/actiontake.hpp"

#include "formatting.hpp"

namespace MWGui
{

    BookWindow::BookWindow ()
        : WindowBase("openmw_book.layout")
        , mCurrentPage(0)
        , mTakeButtonShow(true)
        , mTakeButtonAllowed(true)
    {
        getWidget(mCloseButton, "CloseButton");
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onCloseButtonClicked);

        getWidget(mTakeButton, "TakeButton");
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onTakeButtonClicked);

        getWidget(mNextPageButton, "NextPageBTN");
        mNextPageButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onNextPageButtonClicked);

        getWidget(mPrevPageButton, "PrevPageBTN");
        mPrevPageButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onPrevPageButtonClicked);

        getWidget(mLeftPageNumber, "LeftPageNumber");
        getWidget(mRightPageNumber, "RightPageNumber");

        getWidget(mLeftPage, "LeftPage");
        getWidget(mRightPage, "RightPage");

        adjustButton(mCloseButton);
        adjustButton(mTakeButton);
        adjustButton(mNextPageButton);
        adjustButton(mPrevPageButton);

        mLeftPage->setNeedMouseFocus(true);
        mLeftPage->eventMouseWheel += MyGUI::newDelegate(this, &BookWindow::onMouseWheel);
        mRightPage->setNeedMouseFocus(true);
        mRightPage->eventMouseWheel += MyGUI::newDelegate(this, &BookWindow::onMouseWheel);

        if (mNextPageButton->getSize().width == 64)
        {
            // english button has a 7 pixel wide strip of garbage on its right edge
            mNextPageButton->setSize(64-7, mNextPageButton->getSize().height);
            mNextPageButton->setImageCoord(MyGUI::IntCoord(0,0,64-7,mNextPageButton->getSize().height));
        }

        center();
    }

    void BookWindow::onMouseWheel(MyGUI::Widget *_sender, int _rel)
    {
        if (_rel < 0)
            nextPage();
        else
            prevPage();
    }

    void BookWindow::clearPages()
    {
        mPages.clear();
    }

    void BookWindow::openBook (MWWorld::Ptr book, bool showTakeButton)
    {
        mBook = book;

        clearPages();
        mCurrentPage = 0;

        MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);

        MWWorld::LiveCellRef<ESM::Book> *ref = mBook.get<ESM::Book>();

        Formatting::BookFormatter formatter;
        mPages = formatter.markupToWidget(mLeftPage, ref->mBase->mText);
        formatter.markupToWidget(mRightPage, ref->mBase->mText);

        updatePages();

        setTakeButtonShow(showTakeButton);
    }

    void BookWindow::exit()
    {
        // no 3d sounds because the object could be in a container.
        MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Book);
    }

    void BookWindow::setTakeButtonShow(bool show)
    {
        mTakeButtonShow = show;
        mTakeButton->setVisible(mTakeButtonShow && mTakeButtonAllowed);
    }

    void BookWindow::setInventoryAllowed(bool allowed)
    {
        mTakeButtonAllowed = allowed;
        mTakeButton->setVisible(mTakeButtonShow && mTakeButtonAllowed);
    }

    void BookWindow::onCloseButtonClicked (MyGUI::Widget* sender)
    {
        exit();
    }

    void BookWindow::onTakeButtonClicked (MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getSoundManager()->playSound("Item Book Up", 1.0, 1.0);

        MWWorld::ActionTake take(mBook);
        take.execute (MWMechanics::getPlayer());

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Book);
    }

    void BookWindow::onNextPageButtonClicked (MyGUI::Widget* sender)
    {
        nextPage();
    }

    void BookWindow::onPrevPageButtonClicked (MyGUI::Widget* sender)
    {
        prevPage();
    }

    void BookWindow::updatePages()
    {
        mLeftPageNumber->setCaption( MyGUI::utility::toString(mCurrentPage*2 + 1) );
        mRightPageNumber->setCaption( MyGUI::utility::toString(mCurrentPage*2 + 2) );

        //If it is the last page, hide the button "Next Page"
        if (   (mCurrentPage+1)*2 == mPages.size()
            || (mCurrentPage+1)*2 == mPages.size() + 1)
        {
            mNextPageButton->setVisible(false);
        } else {
            mNextPageButton->setVisible(true);
        }
        //If it is the fist page, hide the button "Prev Page"
        if (mCurrentPage == 0) {
            mPrevPageButton->setVisible(false);
        } else {
            mPrevPageButton->setVisible(true);
        }

        if (mPages.empty())
            return;

        MyGUI::Widget * paper;

        paper = mLeftPage->getChildAt(0);
        paper->setCoord(paper->getPosition().left, -mPages[mCurrentPage*2].first,
                paper->getWidth(), mPages[mCurrentPage*2].second);

        paper = mRightPage->getChildAt(0);
        if ((mCurrentPage+1)*2 <= mPages.size())
        {
            paper->setCoord(paper->getPosition().left, -mPages[mCurrentPage*2+1].first,
                    paper->getWidth(), mPages[mCurrentPage*2+1].second);
            paper->setVisible(true);
        }
        else
        {
            paper->setVisible(false);
        }
    }

    void BookWindow::adjustButton (Gui::ImageButton* button)
    {
        MyGUI::IntSize diff = button->getSize() - button->getRequestedSize();
        button->setSize(button->getRequestedSize());

        if (button->getAlign().isRight())
            button->setPosition(button->getPosition() + MyGUI::IntPoint(diff.width,0));
    }

    void BookWindow::nextPage()
    {
        if ((mCurrentPage+1)*2 < mPages.size())
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book page2", 1.0, 1.0);

            ++mCurrentPage;

            updatePages();
        }
    }
    void BookWindow::prevPage()
    {
        if (mCurrentPage > 0)
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book page", 1.0, 1.0);

            --mCurrentPage;

            updatePages();
        }
    }

}
