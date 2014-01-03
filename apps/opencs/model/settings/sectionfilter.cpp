#include "sectionfilter.hpp"

CSMSettings::SectionFilter::SectionFilter (const QString &sectionName,
                                           QObject *parent)
    : QSortFilterProxyModel (parent)
{
    setSourceModel(CSMSettings::UserSettings::instance().settingModel());
    setFilterRegExp(sectionName);
    setFilterKeyColumn(4);
    setDynamicSortFilter(true);
}

void CSMSettings::SectionFilter::createSetting (const QString &name,
                                                const QString &value)
{
    CSMSettings::UserSettings::instance().settingModel()->
            defineSetting(name, filterRegExp().pattern(), value);
}

QStringList CSMSettings::SectionFilter::valueList (const QString &settingName)
{
    return CSMSettings::UserSettings::instance().settingModel()->
            getSettingValueList(settingName, filterRegExp().pattern());
}
