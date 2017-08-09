
#include "dialogue.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>
#include <QListWidgetItem>

#include "../../model/prefs/state.hpp"

#include "page.hpp"
#include "keybindingpage.hpp"
#include "contextmenulist.hpp"

void CSVPrefs::Dialogue::buildCategorySelector (QSplitter *main)
{
    CSVPrefs::ContextMenuList* list = new CSVPrefs::ContextMenuList (main);
    list->setMinimumWidth (50);
    list->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

    list->setSelectionBehavior (QAbstractItemView::SelectItems);

    main->addWidget (list);

    QFontMetrics metrics (QApplication::font());

    int maxWidth = 1;

    for (CSMPrefs::State::Iterator iter = CSMPrefs::get().begin(); iter!=CSMPrefs::get().end();
        ++iter)
    {
        QString label = QString::fromUtf8 (iter->second.getKey().c_str());
        maxWidth = std::max (maxWidth, metrics.width (label));

        list->addItem (label);
    }

    list->setMaximumWidth (maxWidth + 10);

    connect (list, SIGNAL (currentItemChanged (QListWidgetItem *, QListWidgetItem *)),
        this, SLOT (selectionChanged (QListWidgetItem *, QListWidgetItem *)));
}

void CSVPrefs::Dialogue::buildContentArea (QSplitter *main)
{
    mContent = new QStackedWidget (main);
    mContent->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Expanding);

    main->addWidget (mContent);
}

CSVPrefs::PageBase *CSVPrefs::Dialogue::makePage (const std::string& key)
{
    // special case page code goes here
    if (key == "Key Bindings")
        return new KeyBindingPage(CSMPrefs::get()[key], mContent);
    else
        return new Page (CSMPrefs::get()[key], mContent);
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

CSVPrefs::Dialogue::~Dialogue()
{
    if (isVisible())
        CSMPrefs::State::get().save();
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

void CSVPrefs::Dialogue::selectionChanged (QListWidgetItem *current, QListWidgetItem *previous)
{
    if (current)
    {
        std::string key = current->text().toUtf8().data();

        for (int i=0; i<mContent->count(); ++i)
        {
            PageBase& page = dynamic_cast<PageBase&> (*mContent->widget (i));

            if (page.getCategory().getKey()==key)
            {
                mContent->setCurrentIndex (i);
                return;
            }
        }

        PageBase *page = makePage (key);
        mContent->setCurrentIndex (mContent->addWidget (page));
    }
}
