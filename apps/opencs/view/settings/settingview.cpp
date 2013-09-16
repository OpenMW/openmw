#include <QSortFilterProxyModel>

#include "settingview.hpp"
#include "settingbox.hpp"
#include "settingwidget.hpp"
#include "../../model/setting.hpp"

void CSVSettings::SettingView::buildFilter(const QString &viewName, QSortFilterProxyModel *settingModel)
{
    mSettingFilter = new QSortFilterProxyModel (this);
    mSettingFilter->setFilterFixedString (vieweName);
    mSettingFilter->setFilterKeyColumn (0);
    mSettingFilter->setDynamicSortFilter (true);
}
