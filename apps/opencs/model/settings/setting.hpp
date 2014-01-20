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

    struct Setting
    {
        /// list of values for setting validation / list widgets
        QStringList valueList;

        /// input mask for line edit widgets
        QString inputMask;

        /// map of settings for which the setting acts as a proxy
        /// with accompanying values which correspond to the proxy
        /// widget's value list.
        ProxyMap proxyMap;

        /// default value for the setting;
        QString defaultValue;

        /// name of section/page to which the setting belongs
        QString pageName;

        /// name of the setting
        QString settingName;

        /// type of widget used to represent the setting
        CSVSettings::ViewType viewType;

        /// Indicates whether the setting can accept multiple values.
        bool isMultiValue;

        /// indicate whther the widget is oriented horzintally or vertically
        /// in it's view.  Applies to settings that require multiple instances
        /// of the widget to represent values (radiobutton, checkbox, togglebutton)
        /// Value is true by default.
        bool isHorizontal;

    public:

        explicit Setting(CSVSettings::ViewType viewType,
                         const QString &name, const QString &page)
            : isHorizontal (true), isMultiValue (false),
              viewType (viewType), pageName (page), defaultValue (""),
              inputMask (""), settingName (name)
        {}
    };
}

#endif // SETTING_HPP
