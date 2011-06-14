#include "messagebox.hpp"

using namespace MWGui;

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
}
