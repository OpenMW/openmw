#ifndef CSMSETTINGS_SETTING_HPP
#define CSMSETTINGS_SETTING_HPP

#include <QObject>
#include <QVariant>

#include "../../view/settings/support.hpp"

class QStandardItem;

namespace CSMSettings
{

    typedef QList <QStandardItem *> SettingRowList;

    class Setting : public QObject
    {
        Q_OBJECT

        SettingRowList mSettingRow;

    public:
        explicit Setting(QObject *parent = 0);

        ///boilerplate convenience functions
        QString name() const
            { return property (Property_Name).toString(); }

        void setName (const QString &value)
            { setProperty(Property_Name, value); }

        QString page() const
            { return property (Property_Page).toString(); }

        void setPage (const QString &value)
            { setProperty (Property_Page, value); }

        QString defaultValue() const
            { return property (Property_DefaultValue).toString(); }

        void setDefaultValue (const QString &value)
            { setProperty (Property_DefaultValue, value); }

        QString delimiter() const
        { return property (Property_Delimiter).toString(); }

        void setDelimiter (const QString &value)
        { setProperty (Property_Delimiter, value); }

        CSVSettings::ViewType viewType() const
            { return static_cast<CSVSettings::ViewType>
                                       (property (Property_ViewType).toInt()); }

        void setViewType (CSVSettings::ViewType value)
            { setProperty (Property_ViewType,
                          QVariant(static_cast<int>(value)).toString()); }

        int viewRow() const
            { return property (Property_ViewRow).toInt(); }

        void setViewRow (int value)
            { setProperty (Property_ViewRow, QVariant(value).toString()); }

        int viewColumn() const
            { return property (Property_ViewColumn).toInt(); }

        void setViewColumn (int value)
            { setProperty (Property_ViewColumn, QVariant(value).toString()); }

        int widgetWidth() const
            { return property (Property_WidgetWidth).toInt(); }

        void setWidgetWidth (int value)
            { setProperty (Property_WidgetWidth, QVariant(value).toString()); }

        bool isMultiValue() const
            { return property (Property_IsMultiValue).toBool(); }

        void setMultiValue (bool value)
            { setProperty (Property_IsMultiValue, QVariant(value).toString()); }

        bool isMultiLine() const
            { return property (Property_IsMultiLine).toBool(); }

        void setMultiLine (bool value)
            { setProperty (Property_IsMultiLine, QVariant(value).toString()); }

        bool isHorizontal() const
            { return property (Property_IsHorizontal).toBool(); }

        void setHorizontal (bool value)
            { setProperty (Property_IsHorizontal, QVariant(value).toString()); }

        const QStringList &declaredValues() const
            { return propertyList (PropertyList_DeclaredValues); }

        const QStringList &definedValues() const
            { return propertyList (PropertyList_DefinedValues); }

        ///returns the specified property value
        const QVariant &property (SettingProperty prop) const;

        ///returns the QStringList corresponding to the child of the first
        ///setting property of the row
        const QStringList &propertyList (SettingPropertyList) const;

        ///returns the entire setting values (a model row)
        const SettingRowList &settingRow () const
                                                        { return mSettingRow; }

        void setPropertyList (CSMSettings::SettingPropertyList propertyList,
                          const QStringList &list);

        void setProperty (int column, QString value);
        bool setProperty (int column, QStandardItem *item);
    };
}

#endif // CSMSETTINGS_SETTING_HPP
