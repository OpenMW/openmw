#include "bookwindow.hpp"

#include <boost/lexical_cast.hpp>

#include <components/esm/loadbook.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actiontake.hpp"

#include "formatting.hpp"

namespace MWGui
{

    BookWindow::BookWindow ()
        : WindowBase("openmw_book.layout")
        , mTakeButtonShow(true)
        , mTakeButtonAllowed(true)
        , mCurrentPage(0)
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
        for (std::vector<MyGUI::Widget*>::iterator it=mPages.begin();
            it!=mPages.end(); ++it)
        {
            MyGUI::Gui::getInstance().destroyWidget(*it);
        }
        mPages.clear();
    }

    void BookWindow::open (MWWorld::Ptr book)
    {
        mBook = book;

        clearPages();
        mCurrentPage = 0;

        MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);

        MWWorld::LiveCellRef<ESM::Book> *ref = mBook.get<ESM::Book>();

        BookTextParser parser;
        std::vector<std::string> results = parser.split(ref->mBase->mText, mLeftPage->getSize().width, mLeftPage->getSize().height);

        int i=0;
        for (std::vector<std::string>::iterator it=results.begin();
            it!=results.end(); ++it)
        {
            MyGUI::Widget* parent;
            if (i%2 == 0)
                parent = mLeftPage;
            else
                parent = mRightPage;

            MyGUI::Widget* pageWidget = parent->createWidgetReal<MyGUI::Widget>("", MyGUI::FloatCoord(0.0,0.0,1.0,1.0), MyGUI::Align::Default, "BookPage" + boost::lexical_cast<std::string>(i));
            pageWidget->setNeedMouseFocus(false);
            parser.parsePage(*it, pageWidget, mLeftPage->getSize().width);
            mPages.push_back(pageWidget);
            ++i;
        }

        updatePages();

        setTakeButtonShow(true);
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
        take.execute (MWBase::Environment::get().getWorld()->getPlayerPtr());

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
        mLeftPageNumber->setCaption( boost::lexical_cast<std::string>(mCurrentPage*2 + 1) );
        mRightPageNumber->setCaption( boost::lexical_cast<std::string>(mCurrentPage*2 + 2) );

        unsigned int i=0;
        for (std::vector<MyGUI::Widget*>::iterator it = mPages.begin();
            it != mPages.end(); ++it)
        {
            if (mCurrentPage*2 == i || mCurrentPage*2+1 == i)
                (*it)->setVisible(true);
            else
            {
                (*it)->setVisible(false);
            }
            ++i;
        }

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
    }

    void BookWindow::adjustButton (MWGui::ImageButton* button)
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
