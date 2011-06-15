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
    for(it = mMessageBoxes.begin(); it != mMessageBoxes.end(); ++it) {
        if(i == 3) {
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
{
    setText("message", message);
    update(0);
}

void MessageBox::update (int height)
{
    MyGUI::WidgetPtr messageWidget;
    getWidget(messageWidget, "message");
    
    MyGUI::IntSize size = messageWidget->_getTextSize();
    messageWidget->setSize(size);
    size.width += 20; // padding between text and border of the box
    size.height += 20; // same here
    
    MyGUI::IntSize gameWindowSize = mMessageBoxManager.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord;
    coord.left = (gameWindowSize.width - size.width)/2;
    coord.top = (gameWindowSize.height - size.height - height);
    
    
    std::cout << "Setting MainWidget to position (" << coord.left << "|" << coord.top
        << ") and size to (" << size.width << "|" << size.height << ")"
        << " while height is " << height << std::endl;
    
    mMainWidget->setCoord(coord);
    mMainWidget->setSize(size);
    
    mHeight = size.height;
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
