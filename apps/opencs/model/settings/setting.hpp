#ifndef SETTING_HPP
#define SETTING_HPP

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QStringListModel>
#include "apps/opencs/view/settings/support.hpp"

namespace CSMSettings
{
    typedef QMap<QString, QStringList> ProxyMap;

    class Setting : public QObject
    {
        Q_OBJECT

        /// value stringlist
        QStringList mValues;

        /// list of values for setting validation / list widgets
        QStringList mValueList;

        /// input mask for line edit widgets
        QString mInputMask;

        /// map of settings for which the setting acts as a proxy
        /// with accompanying values which correspond to the proxy
        /// widget's value list.
        ProxyMap mProxyMap;

        /// default value for the setting;
        QString mDefaultValue;

        /// name of section to which the setting belongs
        QString mSectionName;

        /// type of widget used to represent the setting
        CSVSettings::WidgetType mWidgetType;

        /// indicate whther the widget is oriented horzintally or vertically
        /// in it's view.  Applies to settings that require multiple instances
        /// of the widget to represent values (radiobutton, checkbox, togglebutton)
        /// Value is true by default.
        bool mIsHorizontal;

        static QStringList sColumnNames;
        static int sColumnCount;

    public:

        explicit Setting(const QString &name, const QString &section,
                         const QString &defaultValue, QObject *parent = 0);

        void clearValues();

        ///getter functions
        const QStringList &values() const       { return mValues; }
        QStringList &values()                   { return mValues; }
        const QStringList &valueList() const    { return mValueList; }
        QStringList &valueList()                { return mValueList; }
        QString inputMask() const               { return mInputMask; }
        QString sectionName() const             { return mSectionName; }
        QString name() const                    { return objectName(); }
        const ProxyMap &proxyMap() const        { return mProxyMap; }
        ProxyMap &proxyMap()                    { return mProxyMap; }
        QString defaultValue() const            { return mDefaultValue; }
        CSVSettings::WidgetType widgetType() const  { return mWidgetType; }
        bool isHorizontal() const               { return mIsHorizontal; }

        static QStringList columnNames() { return sColumnNames; }
        static int columnCount () { return sColumnCount; }

        ///setter functions
        void addValue (const QString &value);
        void setName (const QString &name)                  { setObjectName (name); }
        void setSectionName (const QString &sectionName)    { mSectionName = sectionName; }
        void setDefaultValue (const QString &defaultValue)  { mDefaultValue = defaultValue; }
        void setValueList (const QStringList &valueList)    { mValueList = valueList; }
        void setInputMask (const QString &mask)             { mInputMask = mask; }
        void setProxyMap (const ProxyMap &proxyMap)         { mProxyMap = proxyMap; }
        void setWidgetType (CSVSettings::WidgetType wType)  { mWidgetType = wType; }
        void setHorizontal (bool value)                     { mIsHorizontal = value; }

        ///function to access the properties by index value
        QVariant item (int index) const;
        void setItem (int index, QVariant value);

    signals:

    public slots:

    };
}

#endif // SETTING_HPP
