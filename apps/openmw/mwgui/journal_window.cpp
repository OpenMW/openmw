#include "journal_window.hpp"
#include "window_manager.hpp"


void formatText()
{
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

    displayLeftText("sdfsdfsdfvf");

    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowChangeCoord = MyGUI::newDelegate(this, &JournalWindow::onWindowResize);
}

void MWGui::JournalWindow::onWindowResize(MyGUI::Window* window)
{
}

void MWGui::JournalWindow::displayLeftText(std::string text)
{
    mLeftTextWidget->removeAllRenderItems();
    mLeftTextWidget->addText(text);
}

void MWGui::JournalWindow::displayRightText(std::string text)
{
    mRightTextWidget->addText(text);
}