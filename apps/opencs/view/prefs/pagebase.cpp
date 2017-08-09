
#include "pagebase.hpp"

#include <QMenu>
#include <QContextMenuEvent>

#include "../../model/prefs/category.hpp"
#include "../../model/prefs/state.hpp"

CSVPrefs::PageBase::PageBase (CSMPrefs::Category& category, QWidget *parent)
: QScrollArea (parent), mCategory (category)
{}

CSMPrefs::Category& CSVPrefs::PageBase::getCategory()
{
    return mCategory;
}

void CSVPrefs::PageBase::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu* menu = new QMenu();

    menu->addAction("Reset category to default", this, SLOT(resetCategory()));
    menu->addAction("Reset all to default", this, SLOT(resetAll()));

    menu->exec(e->globalPos());
    delete menu;
}

void CSVPrefs::PageBase::resetCategory()
{
    CSMPrefs::State::get().resetCategory(getCategory().getKey());
}

void CSVPrefs::PageBase::resetAll()
{
    CSMPrefs::State::get().resetAll();
}
