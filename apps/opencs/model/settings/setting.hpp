#ifndef CSMSETTINGS_SETTING_HPP
#define CSMSETTINGS_SETTING_HPP

#include <QObject>
#include <QVariant>

#include "../../view/settings/support.hpp"
#include "settingitem.hpp"

namespace CSMSettings
{
    typedef QList <SettingItem *> SettingRowList;

    class Setting : public QObject
    {
        Q_OBJECT

        SettingRowList mSettingRow;

    public:
        explicit Setting(QObject *parent = 0);

        ///boilerplate convenience functions
        const QString &name() const
            { return item (Property_Name); }

        const QString &page() const
            { return item (Property_Page); }

        const QString &defaultValue() const
            { return item (Property_DefaultValue); }

        CSVSettings::ViewType viewType() const
            { return item (Property_ViewType); }

        int viewRow() const
            { return item (Property_ViewRow); }

        int viewColumn() const
            { return item (Property_ViewColumn); }

        int widgetWidth() const
            { return item (Property_WidgetWidth); }

        bool isMultiValue() const
            { return item (Property_IsMultiValue); }

        bool isMultiLine() const
            { return item (Property_IsMultiLine); }

        bool isHorizontal() const
            { return item (Property_IsHorizontal); }

        const QStringList &declaredValues() const
            { return itemList (PropertyList_DeclaredValues); }

        const QStringList &definedValues() const
            { return itemList (PropertyList_DefinedValues); }

        ///returns the specified property value
        QVariant item (SettingProperty prop) const;

        ///returns the QStringList corresponding to the child of the first
        ///setting item of the row
        QStringList &itemList (SettingPropertyList) const;

        bool setRowItem (int column);
    };
}

#endif // CSMSETTINGS_SETTING_HPP
