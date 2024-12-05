#include "page.hpp"

#include <utility>
#include <vector>

#include <apps/opencs/view/prefs/pagebase.hpp>

#include <QGridLayout>
#include <QLabel>

#include "../../model/prefs/category.hpp"
#include "../../model/prefs/setting.hpp"

CSVPrefs::Page::Page(CSMPrefs::Category& category, QWidget* parent)
    : PageBase(category, parent)
{
    QWidget* widget = new QWidget(parent);
    mGrid = new QGridLayout(widget);

    for (CSMPrefs::Category::Iterator iter = category.begin(); iter != category.end(); ++iter)
        addSetting(*iter);

    setWidget(widget);
}

void CSVPrefs::Page::addSetting(CSMPrefs::Setting* setting)
{
    const CSMPrefs::SettingWidgets widgets = setting->makeWidgets(this);

    int next = mGrid->rowCount();

    if (widgets.mLabel != nullptr && widgets.mInput != nullptr)
    {
        mGrid->addWidget(widgets.mLabel, next, 0);
        mGrid->addWidget(widgets.mInput, next, 1);
    }
    else if (widgets.mInput != nullptr)
    {
        mGrid->addWidget(widgets.mInput, next, 0, 1, 2);
    }
}
