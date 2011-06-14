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
    mMessageBoxes.push_back(box);
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
    
    MyGUI::WidgetPtr messageWidget;
    getWidget(messageWidget, "message");
    
    MyGUI::IntSize size = messageWidget->_getTextSize();
    size.width += 20;
    size.height += 20;
    
    MyGUI::IntSize gameWindowSize = mMessageBoxManager.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord;
    coord.left = (gameWindowSize.width - size.width)/2;
    coord.top = (gameWindowSize.height - size.height);
    
    
    std::cout << "Setting MainWidget to position (" << coord.left << "|" << coord.top
        << ") and size to (" << size.width << "|" << size.height << ")" << std::endl;
    
    mMainWidget->setCoord(coord);
    mMainWidget->setSize(size);
    
}
