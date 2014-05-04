#ifndef CSMSETTINGS_SETTING_HPP
#define CSMSETTINGS_SETTING_HPP

#include <QStringList>
#include <QMap>
#include "support.hpp"

namespace CSMSettings
{
    //Maps setting id ("page.name") to a list of corresponding proxy values.
    //Order of proxy value stringlists corresponds to order of master proxy's
    //values in it's declared value list
    typedef QMap <QString, QList <QStringList> > ProxyValueMap;

    class Setting
    {
        QList <QStringList> mProperties;
        QStringList mDefaults;

        bool mIsEditorSetting;

        //QString is the setting id in the form of "page.name"
        //QList is  a list of stringlists of proxy values.
        //Order is important!  Proxy stringlists are matched against
        //master values by their position in the QList.
        ProxyValueMap mProxies;

    public:

        explicit Setting(SettingType typ, const QString &settingName,
                         const QString &pageName);

        void addProxy (const Setting *setting, const QStringList &vals);
        void addProxy (const Setting *setting, const QList <QStringList> &list);

        const QList <QStringList> &properties() const   { return mProperties; }
        const ProxyValueMap &proxies() const            { return mProxies; }

        void setColumnSpan (int value);
        int columnSpan() const;

        void setDeclaredValues (QStringList list);
        QStringList declaredValues() const;

        void setDefinedValues (QStringList list);
        QStringList definedValues() const;

        void setDefaultValue (int value);
        void setDefaultValue (double value);
        void setDefaultValue (const QString &value);

        void setDefaultValues (const QStringList &values);
        QStringList defaultValues() const;

        void setDelimiter (const QString &value);
        QString delimiter() const;

        void setEditorSetting (bool state);
        bool isEditorSetting() const;

        void setIsMultiLine (bool state);
        bool isMultiLine() const;

        void setIsMultiValue (bool state);
        bool isMultiValue() const;

        void setMask (const QString &value);
        QString mask() const;

        void setMaximum (int value);
        void setMaximum (double value);
        QString maximum() const;

        void setMinimum (int value);
        void setMinimum (double value);
        QString minimum() const;

        void setName (const QString &value);
        QString name() const;

        void setPage (const QString &value);
        QString page() const;

        void setPrefix (const QString &value);
        QString prefix() const;

        void setRowSpan (const int value);
        int rowSpan() const;

        const ProxyValueMap &proxyLists() const;

        void setSerializable (bool state);
        bool serializable() const;

        void setSpecialValueText (const QString &text);
        QString specialValueText() const;

        void setSingleStep (int value);
        void setSingleStep (double value);
        QString singleStep() const;

        void setSuffix (const QString &value);
        QString suffix() const;

        void setTickInterval (int value);
        int tickInterval() const;

        void setTicksAbove (bool state);
        bool ticksAbove() const;

        void setTicksBelow (bool state);
        bool ticksBelow() const;

        void setViewColumn (int value);
        int viewColumn() const;

        void setViewLocation (int row = -1, int column = -1);

        void setViewRow (int value);
        int viewRow() const;

        void setType (int settingType);
        CSMSettings::SettingType type() const;

        CSVSettings::ViewType viewType() const;

        void setWrapping (bool state);
        bool wrapping() const;

        void setWidgetWidth (int value);
        int widgetWidth() const;

        ///returns the specified property value
        QStringList property (SettingProperty prop) const;

        ///boilerplate code to convert setting values of common types
        void setProperty (SettingProperty prop, bool value);
        void setProperty (SettingProperty prop, int value);
        void setProperty (SettingProperty prop, double value);
        void setProperty (SettingProperty prop, const QString &value);
        void setProperty (SettingProperty prop, const QStringList &value);

        void addProxy (Setting* setting,
                       QMap <QString, QStringList> &proxyMap);

    protected:
        void buildDefaultSetting();
    };
}

#endif // CSMSETTINGS_SETTING_HPP
