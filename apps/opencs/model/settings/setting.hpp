#ifndef CSMSETTINGS_SETTING_HPP
#define CSMSETTINGS_SETTING_HPP

#include <QStringList>
#include <QMap>
#include "support.hpp"

namespace CSMSettings
{
    //QString is the setting id in the form of "page/name"
    //QList is  a list of stringlists of proxy values.
    //Order is important!  Proxy stringlists are matched against
    //master values by their position in the QList.
    typedef QMap <QString, QList <QStringList> > ProxyValueMap;

    ///Setting class is the interface for the User Settings.  It contains
    ///a great deal of boiler plate to provide the core API functions, as
    ///well as the property() functions which use enumeration to be iterable.
    ///This makes the Setting class capable of being manipulated by script.
    ///See CSMSettings::support.hpp for enumerations / string values.
    class Setting
    {
        QList <QStringList> mProperties;
        QStringList mDefaults;

        bool mIsEditorSetting;

        ProxyValueMap mProxies;

    public:

        Setting(SettingType typ, const QString &settingName,
            const QString &pageName, const QString& label = "");

        void addProxy (const Setting *setting, const QStringList &vals);
        void addProxy (const Setting *setting, const QList <QStringList> &list);

        const QList <QStringList> &properties() const   { return mProperties; }
        const ProxyValueMap &proxies() const            { return mProxies; }

        void setColumnSpan (int value);
        int columnSpan() const;

        void setDeclaredValues (QStringList list);
        QStringList declaredValues() const;

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

        void setRange (int min, int max);
        void setRange (double min, double max);

        QString maximum() const;

        QString minimum() const;

        void setName (const QString &value);
        QString name() const;

        void setPage (const QString &value);
        QString page() const;

        void setStyleSheet (const QString &value);
        QString styleSheet() const;

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

        /// This is the text the user gets to see.
        void setLabel (const QString& label);
        QString getLabel() const;

        void setToolTip (const QString& toolTip);
        QString getToolTip() const;

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
