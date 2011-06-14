#include "messagebox.hpp"
//#include "window_manager.hpp"

using namespace MWGui;

void MessageBoxManager::createMessageBox (const std::string& message)
{
    std::cout << "create non-interactive message box" << std::endl;
}

void MessageBoxManager::createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    std::cout << "create interactive message box" << std::endl;
    std::copy (buttons.begin(), buttons.end(), std::ostream_iterator<std::string> (std::cout, ", "));
}
