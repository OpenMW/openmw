#include "journalwindow.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwdialogue/journalentry.hpp"

namespace
{
    struct book
    {
        int endLine;
        std::list<std::string> pages;
    };
}

book formatText(std::string text,book mBook,int maxLine, int lineSize)
{
    //stringList.push_back("");

    int cLineSize = 0;
    int cLine = mBook.endLine +1;
    std::string cString;

    if(mBook.pages.empty())
    {
        cString = "";
        cLine = 0;
    }
    else
    {
        cString = mBook.pages.back() + std::string("\n");
        mBook.pages.pop_back();
    }

    //std::string::iterator wordBegin = text.begin();
    //std::string::iterator wordEnd;

    std::string cText = text;

    while(cText.length() != 0)
    {
        size_t firstSpace = cText.find_first_of(' ');
        if(firstSpace == std::string::npos)
        {
            cString = cString + cText;
            mBook.pages.push_back(cString);
            //TODO:finnish this
            break;
        }
        if(static_cast<int> (firstSpace) + cLineSize <= lineSize)
        {
            cLineSize = firstSpace + cLineSize;
            cString = cString + cText.substr(0,firstSpace +1);
        }
        else
        {
            cLineSize = firstSpace;
            if(cLine +1 <= maxLine)
            {
                cLine = cLine + 1;
                cString = cString + std::string("\n") + cText.substr(0,firstSpace +1);
            }
            else
            {
                cLine = 0;
                mBook.pages.push_back(cString);
                cString = cText.substr(0,firstSpace +1);
            }
        }
        //std::cout << cText << "\n";
        //std::cout << cText.length();
        cText = cText.substr(firstSpace +1,cText.length() - firstSpace -1);
    }
    mBook.endLine = cLine;
    return mBook;
    //std::string
}


MWGui::JournalWindow::JournalWindow (MWBase::WindowManager& parWindowManager)
    : WindowBase("openmw_journal.layout", parWindowManager)
    , mPageNumber(0)
{
    mMainWidget->setVisible(false);
    //setCoord(0,0,498, 342);
    center();

    getWidget(mLeftTextWidget, "LeftText");
    getWidget(mRightTextWidget, "RightText");
    getWidget(mPrevBtn, "PrevPageBTN");
    mPrevBtn->eventMouseButtonClick += MyGUI::newDelegate(this,&MWGui::JournalWindow::notifyPrevPage);
    getWidget(mNextBtn, "NextPageBTN");
    mNextBtn->eventMouseButtonClick += MyGUI::newDelegate(this,&MWGui::JournalWindow::notifyNextPage);
    //MyGUI::ItemBox* list = new MyGUI::ItemBox();
    //list->addItem("qaq","aqzazaz");
    //mScrollerWidget->addChildItem(list);
    //mScrollerWidget->addItem("dserzt",MyGUI::UString("fedgdfg"));
    //mEditWidget->addText("ljblsxdvdsfvgedfvdfvdkjfghldfjgn sdv,nhsxl;vvn lklksbvlksb lbsdflkbdSLKJGBLskdhbvl<kbvlqksbgkqsjhdvb");
    //mEditWidget->show();
    //mEditWidget->setEditStatic(true);
    mLeftTextWidget->addText("left texxxt  ");
    mLeftTextWidget->setEditReadOnly(true);
    mRightTextWidget->setEditReadOnly(true);
    mRightTextWidget->setEditStatic(true);
    mLeftTextWidget->setEditStatic(true);
    mRightTextWidget->addText("Right texxt  ");

    //std::list<std::string> list = formatText("OpenMW rgh dsfg sqef srg ZT  uzql n ZLIEHRF LQSJH GLOIjf qjfmj hslkdgn jlkdjhg qlr isgli shli uhs fiuh qksf cg ksjnf lkqsnbf ksbf sbfkl zbf kuyzflkj sbgdfkj zlfh ozhjfmo hzmfh lizuf rty qzt ezy tkyEZT RYYJ DG fgh  is an open-source implementation of the game engine found in the game Morrowind. This is a dumb test text msodjbg smojg smoig  fiiinnn");
    //std::list<std::string> list = formatText();
    //displayLeftText(list.front());
}

void MWGui::JournalWindow::close()
{
    MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);
}

void MWGui::JournalWindow::open()
{
    mPageNumber = 0;
    MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);
    if(MWBase::Environment::get().getJournal()->begin()!=MWBase::Environment::get().getJournal()->end())
    {
        book journal;
        journal.endLine = 0;

        for(std::deque<MWDialogue::StampedJournalEntry>::const_iterator it = MWBase::Environment::get().getJournal()->begin();it!=MWBase::Environment::get().getJournal()->end();++it)
        {
            std::string a = it->getText(MWBase::Environment::get().getWorld()->getStore());
            journal = formatText(a,journal,10,17);
            journal.endLine = journal.endLine +1;
            journal.pages.back() = journal.pages.back() + std::string("\n");
        }
        //std::string a = MWBase::Environment::get().getJournal()->begin()->getText(MWBase::Environment::get().getWorld()->getStore());
        //std::list<std::string> journal = formatText(a,10,20,1);
        bool left = true;
        for(std::list<std::string>::iterator it = journal.pages.begin(); it != journal.pages.end();++it)
        {
            if(left)
            {
                mLeftPages.push_back(*it);
            }
            else
            {
                mRightPages.push_back(*it);
            }
            left = !left;
        }
        if(!left) mRightPages.push_back("");

        mPageNumber = mLeftPages.size()-1;
        displayLeftText(mLeftPages[mPageNumber]);
        displayRightText(mRightPages[mPageNumber]);

    }
    else
    {
        //std::cout << MWBase::Environment::get().getJournal()->begin()->getText(MWBase::Environment::get().getWorld()->getStore());
    }
}

void MWGui::JournalWindow::displayLeftText(std::string text)
{
    mLeftTextWidget->eraseText(0,mLeftTextWidget->getTextLength());
    mLeftTextWidget->addText(text);
}

void MWGui::JournalWindow::displayRightText(std::string text)
{
    mRightTextWidget->eraseText(0,mRightTextWidget->getTextLength());
    mRightTextWidget->addText(text);
}


void MWGui::JournalWindow::notifyNextPage(MyGUI::Widget* _sender)
{
    if(mPageNumber < int(mLeftPages.size())-1)
    {
        std::string nextSound = "book page2";
        MWBase::Environment::get().getSoundManager()->playSound (nextSound, 1.0, 1.0);
        mPageNumber = mPageNumber + 1;
        displayLeftText(mLeftPages[mPageNumber]);
        displayRightText(mRightPages[mPageNumber]);
    }
}

void MWGui::JournalWindow::notifyPrevPage(MyGUI::Widget* _sender)
{
    if(mPageNumber > 0)
    {
        std::string prevSound = "book page";
        MWBase::Environment::get().getSoundManager()->playSound (prevSound, 1.0, 1.0);
        mPageNumber = mPageNumber - 1;
        displayLeftText(mLeftPages[mPageNumber]);
        displayRightText(mRightPages[mPageNumber]);
    }
}
