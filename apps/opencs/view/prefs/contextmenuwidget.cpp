#include "contextmenuwidget.hpp"

#include <QMenu>
#include <QContextMenuEvent>

#include "../../model/prefs/state.hpp"

CSVPrefs::ContextMenuWidget::ContextMenuWidget(const std::string& category, QWidget* parent)
    :QWidget(parent)
    ,mCategory(category)
{
}

void CSVPrefs::ContextMenuWidget::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu* menu = new QMenu();

    menu->addAction("Reset category to default", this, SLOT(resetCategory()));
    menu->addAction("Reset all to default", this, SLOT(resetAll()));

    menu->exec(e->globalPos());
    delete menu;
}

void CSVPrefs::ContextMenuWidget::resetCategory()
{
    CSMPrefs::State::get().resetCategory(mCategory);
}

void CSVPrefs::ContextMenuWidget::resetAll()
{
    CSMPrefs::State::get().resetAll();
}
