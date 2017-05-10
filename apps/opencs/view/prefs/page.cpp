
#include "page.hpp"

#include <QGridLayout>

#include "../../model/prefs/setting.hpp"
#include "../../model/prefs/category.hpp"
#include "../../view/prefs/contextmenuwidget.hpp"

CSVPrefs::Page::Page (CSMPrefs::Category& category, QWidget *parent)
: PageBase (category, parent)
{
    CSVPrefs::ContextMenuWidget *widget = new CSVPrefs::ContextMenuWidget (category.getKey(), parent);
    mGrid = new QGridLayout (widget);

    for (CSMPrefs::Category::Iterator iter = category.begin(); iter!=category.end(); ++iter)
        addSetting (*iter);

    // HACK to get widget to consume all available page space so context menu clicks
    // will trigger, but so that setting widgets still only take up the left hand side
    QWidget* emptyColumn = new QWidget();
    mGrid->addWidget(emptyColumn, 0, 2, -1, 1);
    QWidget* emptyRow = new QWidget();
    emptyRow->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    mGrid->addWidget(emptyRow, mGrid->rowCount(), 0, 1, -1);

    setWidgetResizable(true);
    setWidget (widget);
}

void CSVPrefs::Page::addSetting (CSMPrefs::Setting *setting)
{
    std::pair<QWidget *, QWidget *> widgets = setting->makeWidgets (this);

    int next = mGrid->rowCount();

    if (widgets.first)
    {
        mGrid->addWidget (widgets.first, next, 0);
        mGrid->addWidget (widgets.second, next, 1);
    }
    else if (widgets.second)
    {
        mGrid->addWidget (widgets.second, next, 0, 1, 2);
    }
    else
    {
        mGrid->addWidget (new QWidget (this), next, 0);
    }
}
