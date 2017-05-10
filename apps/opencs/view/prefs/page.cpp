
#include "page.hpp"

#include <QGridLayout>

#include "../../model/prefs/setting.hpp"
#include "../../model/prefs/category.hpp"
#include "../../view/prefs/contextmenuwidget.hpp"

CSVPrefs::Page::Page (CSMPrefs::Category& category, QWidget *parent)
: PageBase (category, parent)
{
    // topWidget can expand while widget stays the same size
    // This is so the context menu triggers over the entire page
    // but the user interface looks the same
    CSVPrefs::ContextMenuWidget *topWidget = new CSVPrefs::ContextMenuWidget (category.getKey(), parent);
    topWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget* widget = new QWidget(topWidget);
    mGrid = new QGridLayout (widget);

    for (CSMPrefs::Category::Iterator iter = category.begin(); iter!=category.end(); ++iter)
        addSetting (*iter);

    setWidgetResizable(true);
    setWidget (topWidget);
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
