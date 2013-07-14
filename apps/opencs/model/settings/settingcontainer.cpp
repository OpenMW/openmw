#include "settingcontainer.hpp"

#include <QStringList>

CSMSettings::SettingContainer::SettingContainer(QObject *parent) :
    QObject(parent), mValue (0), mValues (0)
{
}

CSMSettings::SettingContainer::SettingContainer(const QString &value, QObject *parent) :
    QObject(parent), mValue (new QString (value)), mValues (0)
{
}

void CSMSettings::SettingContainer::insert (const QString &value)
{
    if (mValue)
    {
        mValues = new QStringList;
        mValues->push_back (*mValue);
        mValues->push_back (value);

        delete mValue;
        mValue = 0;
    }
    else
    {
        delete mValue;
        mValue = new QString (value);
    }

}

void CSMSettings::SettingContainer::update (const QString &value, int index)
{
    if (isEmpty())
        mValue = new QString(value);

    else if (mValue)
        *mValue = value;

    else if (mValues)
        mValues->replace(index, value);
}

QString CSMSettings::SettingContainer::getValue (int index) const
{
    QString retVal("");

    //if mValue is valid, it's a single-value property.
    //ignore the index and return the value
    if (mValue)
        retVal = *mValue;

    //otherwise, if it's a multivalued property
    //return the appropriate value at the index
    else if (mValues)
    {
        if (index == -1)
            retVal = mValues->at(0);

        else if (index < mValues->size())
            retVal = mValues->at(index);
    }

    return retVal;
}

int CSMSettings::SettingContainer::count () const
{
    int retVal = 0;

    if (!isEmpty())
    {
        if (mValues)
            retVal = mValues->size();
        else
            retVal = 1;
    }

    return retVal;
}
