#ifndef SETTINGSITEM_HPP
#define SETTINGSITEM_HPP

#include <QObject>
#include "support.hpp"
#include "settingcontainer.hpp"

namespace CSMSettings
{
    /// Represents a setting including metadata
    /// (valid values, ranges, defaults, and multivalue status
    class SettingsItem : public SettingContainer
    {
        QStringPair *mValuePair;
        QStringList *mValueList;
        bool mIsMultiValue;
        QString mDefaultValue;

    public:
        explicit SettingsItem(QString name, bool isMultiValue,
                              const QString& defaultValue, QObject *parent = 0)
            : SettingContainer(defaultValue, parent),
              mIsMultiValue (isMultiValue), mValueList (0),
              mValuePair (0), mDefaultValue (defaultValue)
        {
            QObject::setObjectName(name);
        }

        /// updateItem overloads for updating setting value
        /// provided a list of values (multi-valued),
        /// a specific value
        /// or an index value corresponding to the mValueList
        bool updateItem (const QStringList *values);
        bool updateItem (const QString &value);
        bool updateItem (int valueListIndex);

        /// retrieve list of valid values for setting
        inline QStringList *getValueList()                  { return mValueList; }

        /// write list of valid values for setting
        inline void setValueList (QStringList *valueList)   { mValueList = valueList; }

        /// valuePair used for spin boxes (max / min)
        inline QStringPair *getValuePair()                  { return mValuePair; }

        /// set value range (spinbox / integer use)
        inline void setValuePair (QStringPair valuePair)
        {
          delete mValuePair;
          mValuePair = new QStringPair(valuePair);
        }

        inline bool isMultivalue ()                         { return mIsMultiValue; }

        void setDefaultValue (const QString &value);
        QString getDefaultValue () const;

    private:

        /// Verifies that the supplied value is one of the following:
        ///  1.  Within the limits of the value pair (min / max)
        ///  2.  One of the values indicated in the value list
        bool validate (const QString &value);
    };
}
#endif // SETTINGSITEM_HPP

