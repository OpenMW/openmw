#include "bookwindow.hpp"

using namespace MWGui;

BookWindow::BookWindow(WindowManager& parWindowManager) :
    WindowBase("openmw_book_layout.xml", parWindowManager)
{
    //setVisible(false);
    center();
}

void BookWindow::open(MWWorld::Ptr book)
{
}

