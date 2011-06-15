#include "messagebox.hpp"

using namespace MWGui;

MessageBoxManager::MessageBoxManager (WindowManager *windowManager)
{
    mWindowManager = windowManager;
}

void MessageBoxManager::createMessageBox (const std::string& message)
{
    std::cout << "create non-interactive message box" << std::endl;
    MessageBox *box = new MessageBox(*this, message);
    
    mMessageBoxes.insert(mMessageBoxes.begin(), box);
    int height = box->getHeight();
    std::vector<MessageBox*>::const_iterator it;
    
    int i = 0;
    for(it = mMessageBoxes.begin()+1; it != mMessageBoxes.end(); ++it) {
        if(i == 2) {
            (*it)->del();
            break;
        }
        else {
            (*it)->update(height);
            height += (*it)->getHeight();
            i++;
        }
    }
}

void MessageBoxManager::createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    std::cout << "create interactive message box" << std::endl;
    std::copy (buttons.begin(), buttons.end(), std::ostream_iterator<std::string> (std::cout, ", "));
}

MessageBox::MessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message)
  : Layout("openmw_messagebox_layout.xml")
  , mMessageBoxManager(parMessageBoxManager)
  , cMessage(message)
{
    mFixedWidth = 300;
    mBottomPadding = 20;
    
    getWidget(mMessageWidget, "message");
    
    mMessageWidget->setOverflowToTheLeft(true);
    mMessageWidget->addText(cMessage);
    
    MyGUI::IntSize size;
    size.width = mFixedWidth; // fiexd width
    size.height = 100; // dummy
    
    MyGUI::IntCoord coord;
    coord.left = 10; // dummy
    coord.top = 10; // dummy

    mMessageWidget->setSize(size);
    
    MyGUI::IntSize textSize = mMessageWidget->_getTextSize();
    size.height = mHeight = textSize.height + 20; // this is the padding between the text and the box
    
    mMainWidget->setSize(size);
    size.width -= 5; // this is to center the text (see messagebox_layout.xml, Widget type="Edit" position="-2 -3 0 0")
    mMessageWidget->setSize(size);
    
    update(0);
}

void MessageBox::update (int height)
{
    MyGUI::IntSize gameWindowSize = mMessageBoxManager.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord;
    coord.left = (gameWindowSize.width - mFixedWidth)/2;
    coord.top = (gameWindowSize.height - mHeight - height - mBottomPadding);
    
    MyGUI::IntSize size;
    size.width = mFixedWidth;
    size.height = mHeight;
    
    mMainWidget->setCoord(coord);
    mMainWidget->setSize(size);
    mMainWidget->setVisible(true);
}

void MessageBox::del ()
{
    // i dont know how to destroy, but therefor i will just set height and width to zero
    MyGUI::IntSize size;
    size.width = size.height = 0;
    mMainWidget->setSize(size);
}

int MessageBox::getHeight ()
{
    return mHeight+20; // 20 is the padding between this and the next MessageBox
}
