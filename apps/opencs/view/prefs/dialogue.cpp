
#include "dialogue.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>

#include "../../model/prefs/state.hpp"

void CSVPrefs::Dialogue::buildCategorySelector (QSplitter *main)
{
    mList = new QListWidget (main);
    mList->setMinimumWidth (50);
    mList->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

    mList->setSelectionBehavior (QAbstractItemView::SelectItems);

    main->addWidget (mList);

    /// \todo connect to selection signal
}

void CSVPrefs::Dialogue::buildContentArea (QSplitter *main)
{
    mContent = new QStackedWidget (main);
    mContent->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);

    main->addWidget (mContent);
}

CSVPrefs::Dialogue::Dialogue()
{
    setWindowTitle ("User Settings");

    setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    setMinimumSize (600, 400);

    QSplitter *main = new QSplitter (this);

    setCentralWidget (main);
    buildCategorySelector (main);
    buildContentArea (main);
}

void CSVPrefs::Dialogue::closeEvent (QCloseEvent *event)
{
    QMainWindow::closeEvent (event);

    CSMPrefs::State::get().save();
}

void CSVPrefs::Dialogue::show()
{
    if (QWidget *active = QApplication::activeWindow())
    {
        // place at the centre of the window with focus
        QSize size = active->size();
        move (active->geometry().x()+(size.width() - frameGeometry().width())/2,
            active->geometry().y()+(size.height() - frameGeometry().height())/2);
    }
    else
    {
        // otherwise place at the centre of the screen
        QPoint screenCenter = QApplication::desktop()->screenGeometry().center();
        move (screenCenter - QPoint(frameGeometry().width()/2, frameGeometry().height()/2));
    }

    QWidget::show();
}
