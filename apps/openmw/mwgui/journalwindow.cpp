#include "journalwindow.hpp"
#include "window_manager.hpp"
#include "../mwdialogue/journal.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

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


MWGui::JournalWindow::JournalWindow (WindowManager& parWindowManager)
    : WindowBase("openmw_journal_layout.xml", parWindowManager)
    , lastPos(0)
{
    //setCoord(0,0,498, 342);
    center();

    getWidget(mLeftTextWidget, "LeftText");
    getWidget(mRightTextWidget, "RightText");
    getWidget(mPrevBtn, "PrevPageBTN");
    mPrevBtn->eventMouseButtonClick = MyGUI::newDelegate(this,&MWGui::JournalWindow::notifyPrevPage);
    getWidget(mNextBtn, "NextPageBTN");
    mNextBtn->eventMouseButtonClick = MyGUI::newDelegate(this,&MWGui::JournalWindow::notifyNextPage);
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

    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowChangeCoord = MyGUI::newDelegate(this, &JournalWindow::onWindowResize);
}

void MWGui::JournalWindow::open()
{
    mPageNumber = 0;
    if(mWindowManager.getEnvironment().mJournal->begin()!=mWindowManager.getEnvironment().mJournal->end())
    {
        book journal;
        journal.endLine = 0;

        for(std::deque<MWDialogue::StampedJournalEntry>::const_iterator it = mWindowManager.getEnvironment().mJournal->begin();it!=mWindowManager.getEnvironment().mJournal->end();it++)
        {
            std::string a = it->getText(mWindowManager.getEnvironment().mWorld->getStore());
            journal = formatText(a,journal,10,17);
            journal.endLine = journal.endLine +1;
            journal.pages.back() = journal.pages.back() + std::string("\n");
        }
        //std::string a = mWindowManager.getEnvironment().mJournal->begin()->getText(mWindowManager.getEnvironment().mWorld->getStore());
        //std::list<std::string> journal = formatText(a,10,20,1);
        bool left = true;
        for(std::list<std::string>::iterator it = journal.pages.begin(); it != journal.pages.end();it++)
        {
            if(left)
            {
                leftPages.push_back(*it);
            }
            else
            {
                rightPages.push_back(*it);
            }
            left = !left;
        }
        if(!left) rightPages.push_back("");

        mPageNumber = leftPages.size()-1;
        displayLeftText(leftPages[mPageNumber]);
        displayRightText(rightPages[mPageNumber]);

    }
    else
    {
        //std::cout << mWindowManager.getEnvironment().mJournal->begin()->getText(mWindowManager.getEnvironment().mWorld->getStore());
    }
}

void MWGui::JournalWindow::onWindowResize(MyGUI::Window* window)
{
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


void MWGui::JournalWindow::notifyNextPage(MyGUI::WidgetPtr _sender)
{
    if(mPageNumber < int(leftPages.size())-1)
    {
        mPageNumber = mPageNumber + 1;
        displayLeftText(leftPages[mPageNumber]);
        displayRightText(rightPages[mPageNumber]);
    }
}

void MWGui::JournalWindow::notifyPrevPage(MyGUI::WidgetPtr _sender)
{
    if(mPageNumber > 0)
    {
        mPageNumber = mPageNumber - 1;
        displayLeftText(leftPages[mPageNumber]);
        displayRightText(rightPages[mPageNumber]);
    }
}
