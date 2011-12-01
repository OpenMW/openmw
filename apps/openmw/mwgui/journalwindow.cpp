#include "journalwindow.hpp"
#include "window_manager.hpp"


std::list<std::string> formatText(std::string text)
{
    std::cout << "\n \n \n";
    std::list<std::string> stringList;
    //stringList.push_back("");
    int maxLine = 10;
    int lineSize = 20;

    int cLineSize = 0;
    int cLine = 0;
    std::string cString;

    std::string::iterator wordBegin = text.begin();
    std::string::iterator wordEnd;

    std::string cText = text;

    while(cText.length() != 0)
    {
        std::cout << "a";
        size_t firstSpace = cText.find_first_of(' ');
        if(firstSpace == std::string::npos)
        {
            cString = cString + cText;
            stringList.push_back(cString);
            //TODO:finnish this
            std::cout << "brerak?";
            break;
        }
        std::cout << "notbreak";
        if(firstSpace + cLineSize <= lineSize)
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
                stringList.push_back(cString);
                cString = cText.substr(0,firstSpace +1);
            }
        }
        //std::cout << cText << "\n";
        std::cout << cText.length();
        if(firstSpace == cText.length()) std::cout << "maxi error en veu";
        cText = cText.substr(firstSpace +1,cText.length() - firstSpace -1);
    }
    return stringList;
    //std::string 
}


MWGui::JournalWindow::JournalWindow (WindowManager& parWindowManager)
    : WindowBase("openmw_journal_layout.xml", parWindowManager)
    , lastPos(0)
{
    setCoord(0,0,498, 342);

    getWidget(mLeftTextWidget, "LeftText");
    getWidget(mRightTextWidget, "RightText");
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

    std::list<std::string> list = formatText("OpenMW rgh dsfg sqef srg ZT  uzql n ZLIEHRF LQSJH GLOIjf qjfmj hslkdgn jlkdjhg qlr isgli shli uhs fiuh qksf cg ksjnf lkqsnbf ksbf sbfkl zbf kuyzflkj sbgdfkj zlfh ozhjfmo hzmfh lizuf rty qzt ezy tkyEZT RYYJ DG fgh  is an open-source implementation of the game engine found in the game Morrowind. This is a dumb test text msodjbg smojg smoig  fiiinnn");
    displayLeftText(list.front());

    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowChangeCoord = MyGUI::newDelegate(this, &JournalWindow::onWindowResize);
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