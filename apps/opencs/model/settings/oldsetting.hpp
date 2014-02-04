#ifndef OLDSETTING_HPP
#define OLDSETTING_HPP

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

        /// character(s) which delimit records when concatenated into a single
        /// string (Text view types)
        QString delimiter;

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

        /// Width of the width in characters.  Default settings used if zero.
        int widgetWidth;

        /// Indicates whether the setting can accept multiple values.
        bool isMultiValue;

        /// Indicates whether or not the widget takes a single line or
        /// multiple lines of text (use with ViewType_Text only).
        bool isMultiLineText;

        /// indicate whther the widget is oriented horzintally or vertically
        /// in it's view.  Applies to settings that require multiple instances
        /// of the widget to represent values (radiobutton, checkbox, togglebutton)
        /// Value is true by default.
        bool isHorizontal;

        int viewRow;

        int viewColumn;

    public:

        explicit Setting(CSVSettings::ViewType viewType,
                         const QString &name, const QString &page)
            : isHorizontal (true), isMultiValue (false), isMultiLineText(false),
              viewType (viewType), pageName (page), defaultValue (""),
              inputMask (""), settingName (name), delimiter(';'),
              widgetWidth (0), viewRow (-1), viewColumn (-1)
        {}
    };
}

#endif // OLDSETTING_HPP
