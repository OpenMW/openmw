#include "sectionfilter.hpp"

CSMSettings::SectionFilter::SectionFilter (const QString &sectionName,
                                           QObject *parent)
    : QSortFilterProxyModel (parent)
{
    setSourceModel(CSMSettings::UserSettings::instance().settingModel());
    setFilterRegExp(sectionName);
    setFilterKeyColumn(1);
    setDynamicSortFilter(true);
}

void CSMSettings::SectionFilter::createSetting (const QString &name,
                                                const QString &value,
                                                const QStringList &valueList)
{
    CSMSettings::UserSettings::instance().settingModel()->
            createSetting(name, filterRegExp().pattern(), value, valueList);
}
