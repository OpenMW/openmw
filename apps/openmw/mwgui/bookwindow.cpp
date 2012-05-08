#include "bookwindow.hpp"

#include "formatting.hpp"

#include "../mwbase/environment.hpp"
#include "../mwinput/inputmanager.hpp"
#include "../mwsound/soundmanager.hpp"
#include "../mwworld/actiontake.hpp"

using namespace MWGui;

BookWindow::BookWindow (WindowManager& parWindowManager) :
    WindowBase("openmw_book_layout.xml", parWindowManager)
{
    getWidget(mCloseButton, "CloseButton");
    mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onCloseButtonClicked);

    getWidget(mTakeButton, "TakeButton");
    mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onTakeButtonClicked);

    getWidget(mNextPageButton, "NextPageBTN");
    mNextPageButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onNextPageButtonClicked);

    getWidget(mPrevPageButton, "PrevPageBTN");
    mPrevPageButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BookWindow::onPrevPageButtonClicked);

    center();
}

void BookWindow::open (MWWorld::Ptr book)
{
    MWBase::Environment::get().getSoundManager()->playSound3D (book, "book open", 1.0, 1.0);

    mBook = book;

    ESMS::LiveCellRef<ESM::Book, MWWorld::RefData> *ref =
        mBook.get<ESM::Book>();

    //BookTextParser parser;
    //parser.parse(ref->base->text, 0, 0);
}

void BookWindow::onCloseButtonClicked (MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getSoundManager()->playSound3D (mBook, "book close", 1.0, 1.0);

    MWBase::Environment::get().getInputManager()->setGuiMode(MWGui::GM_Game);
}

void BookWindow::onTakeButtonClicked (MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getSoundManager()->playSound3D (mBook, "Item Book Up", 1.0, 1.0, MWSound::Play_NoTrack);

    MWWorld::ActionTake take(mBook);
    take.execute();

    /// \todo what about scripts?

    MWBase::Environment::get().getInputManager()->setGuiMode (GM_Game);
}

void BookWindow::onNextPageButtonClicked (MyGUI::Widget* _sender)
{
}

void BookWindow::onPrevPageButtonClicked (MyGUI::Widget* _sender)
{
}
