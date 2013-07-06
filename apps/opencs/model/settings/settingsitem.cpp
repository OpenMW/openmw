#include "settingsitem.hpp"

#include <QStringList>

bool CSMSettings::SettingsItem::updateItem (const QStringList *values)
{
    QStringList::ConstIterator it = values->begin();

    //if the item is not multivalued,
    //save the last value passed in the container
    if (!mIsMultiValue)
    {
        it = values->end();
        it--;
    }

    bool isValid = true;
    QString value ("");

    for (; it != values->end(); ++it)
    {
        value = *it;
        isValid = validate(value);

        //skip only the invalid values
        if (!isValid)
            continue;

        insert(value);
    }

    return isValid;
}

bool CSMSettings::SettingsItem::updateItem (const QString &value)
{
    //takes a value or a SettingsContainer and updates itself accordingly
    //after validating the data against it's own definition

    QString newValue = value;

    if (!validate (newValue))
        newValue = mDefaultValue;

    bool success = (getValue() != newValue);

    if (success)
    {
        if (mIsMultiValue)
            insert (newValue);
        else
            update (newValue);
    }
    return success;
}

bool CSMSettings::SettingsItem::updateItem(int valueListIndex)
{
    bool success = false;

    if (mValueList)
    {
        if (mValueList->size() > valueListIndex)
            success = updateItem (mValueList->at(valueListIndex));
    }
    return success;
}

bool CSMSettings::SettingsItem::validate (const QString &value)
{
    //if there is no value list or value pair, there is no validation to do
    bool isValid = !(!mValueList->isEmpty() || mValuePair);

    if (!isValid && !mValueList->isEmpty())
    {
        for (QStringList::Iterator it = mValueList->begin(); it != mValueList->end(); ++it)
    //    foreach (QString listItem, *mValueList)
        {
            isValid = (value == *it);

            if (isValid)
                break;
        }
    }
    else if (!isValid && mValuePair)
    {
        int numVal = value.toInt();

        isValid = (numVal > mValuePair->left.toInt() && numVal < mValuePair->right.toInt());
    }

    return isValid;
}

void CSMSettings::SettingsItem::setDefaultValue (const QString &value)
{
    mDefaultValue = value;
    update (value);
}

QString CSMSettings::SettingsItem::getDefaultValue() const
{
    return mDefaultValue;
}
