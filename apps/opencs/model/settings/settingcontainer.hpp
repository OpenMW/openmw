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

        virtual QString getName() const {return "";}

        void insert (const QString &value);
        void update (const QString &value, int index = 0);

        QString getValue (int index = -1) const;
        inline QStringList *getValues() const  { return mValues; }
        int count() const;

        //test for empty container
        //useful for default-constructed containers returned by QMap when invalid key is passed
        inline bool isEmpty() const            { return (!mValue && !mValues); }
        inline bool isMultiValue() const       { return (mValues); }
    };
}

#endif // SETTINGCONTAINER_HPP
