#ifndef SETTINGSITEM_HPP
#define SETTINGSITEM_HPP

#include <QObject>
#include "support.hpp"
#include "settingcontainer.hpp"

namespace CSMSettings
{
    class SettingsItem : public SettingContainer
    {
        QStringPair *mValuePair;
        QStringList *mValueList;
        bool mIsMultiValue;
        QString mName;
        QString mDefaultValue;

    public:
        explicit SettingsItem(QString name, bool isMultiValue,
                              const QString& defaultValue, QObject *parent = 0)
            : SettingContainer(defaultValue, parent),
              mIsMultiValue (isMultiValue), mValueList (0),
              mName (name), mValuePair (0), mDefaultValue (defaultValue)
        {}

        bool updateItem (const QStringList *values);
        bool updateItem (const QString &value);
        bool updateItem (int valueListIndex);

        inline QStringList *getValueList()                  { return mValueList; }
        inline void setValueList (QStringList *valueList)   { mValueList = valueList; }

        inline QStringPair *getValuePair()                  { return mValuePair; }
        inline void setValuePair (QStringPair valuePair)    { mValuePair = new QStringPair(valuePair); }

        inline QString getName () const                     { return mName; }
        inline bool isMultivalue ()                         { return mIsMultiValue; }

        void setDefaultValue (const QString &value);
        QString getDefaultValue () const;

    private:
        bool validate (const QString &value);
    };
}
#endif // SETTINGSITEM_HPP

