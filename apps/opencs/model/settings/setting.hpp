#ifndef CSMSETTINGS_SETTING_HPP
#define CSMSETTINGS_SETTING_HPP

#include <QObject>
#include <QVariant>
#include <QStringList>

#include "../../view/settings/support.hpp"

class QStandardItem;

namespace CSMSettings
{

    typedef QList <QStandardItem *> RowItemList;

    class Setting : public QObject
    {
        Q_OBJECT

        RowItemList mSettingRow;
        QStringList mTemp;
        QList <QList <QStandardItem *> > mChildRows;

    public:

        explicit Setting();

        explicit Setting(SettingType typ, const QString &name,
                         const QString &page, const QString &defaultValue,
                         QObject *parent = 0);

        Setting (const Setting &copy)
            : mSettingRow (copy.mSettingRow), QObject (copy.parent())
        {}

        Setting operator =(const Setting &rhv) { return Setting (rhv); }

        QStringList declaredValues() const
            { return propertyList (PropertyList_DeclaredValues); }

        QStringList definedValues() const
            { return propertyList (PropertyList_DefinedValues); }

        ///returns the specified property value
        QVariant property (SettingProperty prop) const;

        ///returns the QStringList corresponding to the child of the first
        ///setting property of the row
        QStringList propertyList (SettingPropertyList) const;

        ///returns the entire setting values (a model row)
        const RowItemList &rowList () const         { return mSettingRow; }

        void setPropertyList (CSMSettings::SettingPropertyList propertyList,
                          const QStringList &list);

        void setProperty (int column, QString value);
        void setProperty (int column, int value);
        void setProperty (int column, bool value);
        void setProperty (int column, QStandardItem *item);

        ///boilerplate for convenience functions
        QString page() const     { return property (Property_Page).toString(); }
        QString name() const     { return property (Property_Name).toString(); }
        QString defaultValue()
                         { return property (Property_DefaultValue).toString(); }
        QString delimiter() { return property (Property_Delimiter).toString(); }

        bool isMultiValue() const
                            { return property (Property_IsMultiValue).toBool();}

        bool isMultiLine() const
                            { return property (Property_IsMultiLine).toBool(); }

        int viewRow() const      { return property (Property_ViewRow).toInt(); }
        int viewColumn() const{ return property (Property_ViewColumn).toInt(); }

        const QStringList declarationList() const
                          { return propertyList (PropertyList_DeclaredValues); }

        const QStringList definitionList() const
                           { return propertyList (PropertyList_DefinedValues); }

        const QStringList proxyList() const
                           { return propertyList (PropertyList_Proxies); }

        CSVSettings::ViewType viewType() const
        { return static_cast<CSVSettings::ViewType>
                                        (property(Property_ViewType).toInt()); }
    private:

        void buildDefaultSetting();
    };
}

#endif // CSMSETTINGS_SETTING_HPP
