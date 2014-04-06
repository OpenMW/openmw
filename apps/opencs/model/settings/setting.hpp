#ifndef CSMSETTINGS_SETTING_HPP
#define CSMSETTINGS_SETTING_HPP

#include <QStringList>
#include <QMap>
#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    typedef QList <Setting *> SettingList;

    //StringPair contains page / setting names
    //StringListPairs is a list of master / proxy value mappings, where:
    //   StringListPair.first = master key
    //   StringListPair.second = corresponding proxy value list

    //QPair maps a list of master/proxy values to a specific setting
    typedef QPair <StringPair, StringListPairs> ProxySettingPair;

    // A list of settings and their corresponding lists of proxy values
    // keyed to their respective master values
    typedef QList <ProxySettingPair> ProxySettingPairs;

    struct Setting
    {
        QList <QStringList> mLayout;
        QStringList mDefinitions;
        QStringList mDeclarations;

        /*
        * QStringList is the list of proxy values for one master value
        * Inner list index corresponds to the master value index
        * Outer list index corresponds to proxy setting / selector
        */

        ProxySettingPairs mProxyValueLists;

    public:

        explicit Setting();

        explicit Setting(SettingType typ, const QString &settingName,
                         const QString &pageName);

        void setIsSerialized (bool state);
        bool isSerialized() const;

        void setDeclaredValues (QStringList list);
        const QStringList &declaredValues() const;

        void setDefinedValues (QStringList list);
        const QStringList &definedValues() const;

        void setDefaultValues (const QStringList &values);
        QStringList defaultValues() const;

        void setDelimiter (const QString &value);
        QString delimiter() const;

        void setIsMultiLine (bool state);
        bool isMultiLine() const;

        void setIsMultiValue (bool state);
        bool isMultiValue() const;

        void setName (const QString &value);
        QString name() const;

        void setPage (const QString &value);
        QString page() const;

        const ProxySettingPairs &proxyLists() const;

        void setSerializable (bool state);
        bool serializable() const;

        void setViewColumn (int value);
        int viewColumn() const;

        void setViewRow (int value);
        int viewRow() const;

        void setViewType (int vType);
        CSVSettings::ViewType viewType() const;

        void setWidgetWidth (int value);
        int widgetWidth() const;

        ///returns the specified property value
        QVariant property (SettingProperty prop) const;

        ///boilerplate code to convert setting values of common types
        void setProperty (SettingProperty prop, bool value);
        void setProperty (SettingProperty prop, int value);
        void setProperty (SettingProperty prop, const QVariant &value);
        void setProperty (SettingProperty prop, const QString &value);
        void setProperty (SettingProperty prop, const QStringList &value);

        void addProxy (Setting* setting,
                       QMap <QString, QStringList> &proxyMap);

        void dumpSettingValues();

    protected:
        void buildDefaultSetting();
    };
}
Q_DECLARE_METATYPE(CSMSettings::Setting)

QDataStream &operator <<(QDataStream &stream, const CSMSettings::Setting& setting);
QDataStream &operator >>(QDataStream &stream, CSMSettings::Setting& setting);

#endif // CSMSETTINGS_SETTING_HPP
