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
        explicit SettingItem();

        void setChildValues (const QStringList &list);

        const QStringList &childValues(SettingPropertyList propertyList) const;
        QStringList       childValues (SettingPropertyList propertyList);

        const QStringList   &values() const;
        QStringList         values();

    };
}
#endif // CSMSETTINGS_SETTINGITEM_HPP
