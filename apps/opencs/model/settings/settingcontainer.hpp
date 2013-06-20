#ifndef SETTINGCONTAINER_HPP
#define SETTINGCONTAINER_HPP

#include <QObject>

class QStringList;

namespace CSMSettings
{
    class SettingContainer : public QObject
    {
        Q_OBJECT

        QString *mValue;
        QStringList *mValues;

    public:

        explicit SettingContainer (QObject *parent = 0);
        explicit SettingContainer (const QString &value, QObject *parent = 0);

        /// add a value to the container
        /// multiple values supported
        void insert (const QString &value);

        /// update an existing value
        /// index specifies multiple values
        void update (const QString &value, int index = 0);

        /// return value at specified index
        QString getValue (int index = -1) const;

        /// retrieve list of all values
        inline QStringList *getValues() const  { return mValues; }

        /// return size of list
        int count() const;

        /// test for empty container
        /// useful for default-constructed containers returned by QMap when invalid key is passed
        inline bool isEmpty() const            { return (!mValue && !mValues); }

        inline bool isMultiValue() const       { return (mValues); }
    };
}

#endif // SETTINGCONTAINER_HPP
