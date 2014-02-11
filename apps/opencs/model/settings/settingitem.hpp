#ifndef CSMSETTINGS_SETTINGITEM_HPP
#define CSMSETTINGS_SETTINGITEM_HPP

#include <QStandardItem>

#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    class SettingItem : public QStandardItem
    {
        QStringList mValues;

    public:
        explicit SettingItem(const QString &data);

        void setChildValues (const QStringList &list);

        const QStringList &childValues(SettingPropertyList propertyList) const;
        QStringList       childValues (SettingPropertyList propertyList);

        const QStringList   &values() const;
        QStringList         values();

        QVariant data(int role = Qt::UserRole + 1) const
                                            { return QStandardItem::data(); }

        void setData (const QVariant &value, int role = Qt::UserRole + 1)
                                       { QStandardItem::setData (value, role); }


    };
}
#endif // CSMSETTINGS_SETTINGITEM_HPP
