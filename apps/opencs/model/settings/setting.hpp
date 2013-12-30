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

    public:

        explicit Setting(const QString &name, const QString &section,
                         const QString &defaultValue, QObject *parent = 0);

        ///getter functions
        const QStringList &valueList() const    { return mValueList; }
        QStringList &valueList()                { return mValueList; }
        QString inputMask() const               { return mInputMask; }
        QString name() const                    { return objectName(); }
        const ProxyMap &proxyMap() const        { return mProxyMap; }
        ProxyMap &proxyMap()                    { return mProxyMap; }
        QString defaultValue() const            { return mDefaultValue; }
        CSVSettings::WidgetType widgetType() const  { return mWidgetType; }
        bool isHorizontal() const               { return mIsHorizontal; }

        ///setter functions
        void setValueList (const QStringList &valueList)    { mValueList = valueList; }
        void setInputMask (const QString &mask)             { mInputMask = mask; }
        void setProxyMap (const ProxyMap &proxyMap)         { mProxyMap = proxyMap; }
        void setWidgetType (CSVSettings::WidgetType wType)  { mWidgetType = wType; }

    signals:

    public slots:

    };
}

#endif // SETTING_HPP
