#include "contextmenulist.hpp"

#include <QMenu>
#include <QContextMenuEvent>
#include <QMouseEvent>

#include "../../model/prefs/state.hpp"

CSVPrefs::ContextMenuList::ContextMenuList(QWidget* parent)
    :QListWidget(parent)
{
}

void CSVPrefs::ContextMenuList::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu* menu = new QMenu();

    menu->addAction("Reset category to default", this, SLOT(resetCategory()));
    menu->addAction("Reset all to default", this, SLOT(resetAll()));

    menu->exec(e->globalPos());
    delete menu;
}

void CSVPrefs::ContextMenuList::mousePressEvent(QMouseEvent* e)
{
    // enable all buttons except right click
    // This means that when right-clicking to enable the
    // context menu, the page doesn't switch at the same time.
    if (!(e->buttons() & Qt::RightButton))
    {
        QListWidget::mousePressEvent(e);
    }
}

void CSVPrefs::ContextMenuList::resetCategory()
{
    CSMPrefs::State::get().resetCategory(currentItem()->text().toStdString());
}

void CSVPrefs::ContextMenuList::resetAll()
{
    CSMPrefs::State::get().resetAll();
}
