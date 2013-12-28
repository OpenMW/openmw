#ifndef BINARYWIDGETMODEL_HPP
#define BINARYWIDGETMODEL_HPP

#include <QStringList>
#include <QSortFilterProxyModel>
#include "usersettings.hpp"
namespace CSMSettings
{
    class SectionFilter : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        explicit SectionFilter (const QString &sectionName, QObject *parent)
            : QSortFilterProxyModel (parent)
        {
            setSourceModel(CSMSettings::UserSettings::instance().settingModel());
            setFilterRegExp(sectionName);
            setFilterKeyColumn(1);
            setDynamicSortFilter(true);
        }

        void createSetting (const QString &name, const QString &value, const QStringList &valueList)
        {
            CSMSettings::UserSettings::instance().settingModel()->
                    createSetting(name, filterRegExp().pattern(), value, valueList);
        }
    };

    class BinaryWidgetAdapter : public QSortFilterProxyModel
    {
        Q_OBJECT

        QString mSection;
        QString mSetting;
        QStringList mValueList;
        SectionFilter *mFilter;

    public:

        explicit BinaryWidgetAdapter(SectionFilter *filter,
                                   const QString &settingName,
                                   QObject *parent = 0);

        QStringList valueList () const              { return mValueList; }

        QModelIndex itemIndex      (const QString &item);
        bool insertItem     (const QString &item);
        bool removeItem     (const QString &item);
    };
}
#endif // BINARYWIDGETMODEL_HPP
