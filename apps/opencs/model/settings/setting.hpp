#ifndef SETTING_HPP
#define SETTING_HPP

#include <QObject>
#include <QStringList>
#include <QVariant>

namespace CSMSettings
{

    class Setting : public QObject
    {
        Q_OBJECT

        /// current value of the setting
        QStringList mValues;

        /// list of values for setting validation / list widgets
        QStringList mValueList;

        /// input mask for line edit widgets
        QString mInputMask;

        /// list of settings for which the setting acts as a proxy
        QStringList mProxyList;

        /// default value for the setting;
        QString mDefaultValue;

        /// name of section to which the setting belongs
        QString mSectionName;

    public:
        static QStringList sColumnNames;

    public:

        explicit Setting(const QString &name, const QString &section,
                         const QString &defaultValue, QObject *parent = 0);

        ///getter functions
        QString value (int index = 0) const     { return mValues.at(index); }

        const QStringList &values() const       { return mValues; }
        QStringList &values()                   { return mValues; }

        const QStringList &valueList() const    { return mValueList; }
        QStringList &valueList()                { return mValueList; }

        QString inputMask() const               { return mInputMask; }
        QString sectionName() const             { return mSectionName; }

        QString name() const                    { return objectName(); }

        const QStringList &proxyList() const    { return mProxyList; }
        QStringList &proxyList()                { return mProxyList; }

        QString defaultValue() const            { return mDefaultValue; }

        ///setter functions
        void setValue (const QString &value, int index = 0);
        void setName (const QString &name)                  { setObjectName (name); }
        void setSectionName (const QString &sectionName)    { mSectionName = sectionName; }
        void setDefaultValue (const QString &defaultValue)  { mDefaultValue = defaultValue; }
        void setValues (const QStringList &values)          { mValues = values; }
        void setValueList (const QStringList &valueList)    { mValueList = valueList; }
        void setInputMask (const QString &mask)             { mInputMask = mask; }
        void setProxyList (const QStringList &proxyList)    { mProxyList = proxyList; }

        ///function to access the properties by index value
        QVariant item (int index) const;
        void setItem (int index, QVariant value);

    signals:

    public slots:

    };
}
#endif // SETTING_HPP
