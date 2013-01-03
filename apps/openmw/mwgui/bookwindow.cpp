#include "bookwindow.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actiontake.hpp"
#include "../mwworld/player.hpp"

#include "formatting.hpp"

using namespace MWGui;

BookWindow::BookWindow (MWBase::WindowManager& parWindowManager) :
    WindowBase("openmw_book.layout", parWindowManager)
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

    center();
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
        parser.parse(*it, pageWidget, mLeftPage->getSize().width);
        mPages.push_back(pageWidget);
        ++i;
    }

    updatePages();

    setTakeButtonShow(true);
}

void BookWindow::setTakeButtonShow(bool show)
{
    mTakeButton->setVisible(show);
}

void BookWindow::onCloseButtonClicked (MyGUI::Widget* sender)
{
    // no 3d sounds because the object could be in a container.
    MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);

    mWindowManager.removeGuiMode(GM_Book);
}

void BookWindow::onTakeButtonClicked (MyGUI::Widget* sender)
{
    MWBase::Environment::get().getSoundManager()->playSound ("Item Book Up", 1.0, 1.0, MWBase::SoundManager::Play_NoTrack);

    MWWorld::ActionTake take(mBook);
    take.execute (MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

    mWindowManager.removeGuiMode(GM_Book);
}

void BookWindow::onNextPageButtonClicked (MyGUI::Widget* sender)
{
    if ((mCurrentPage+1)*2 < mPages.size())
    {
        MWBase::Environment::get().getSoundManager()->playSound ("book page2", 1.0, 1.0);

        ++mCurrentPage;

        updatePages();
    }
}

void BookWindow::onPrevPageButtonClicked (MyGUI::Widget* sender)
{
    if (mCurrentPage > 0)
    {
        MWBase::Environment::get().getSoundManager()->playSound ("book page", 1.0, 1.0);

        --mCurrentPage;

        updatePages();
    }
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
}
