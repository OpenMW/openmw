#include "setting.hpp"
#include <QStringList>

QStringList CSMSettings::Setting::sColumnNames;

CSMSettings::Setting::Setting (const QString &name, const QString &section,
                               const QString &defaultValue, QObject *parent)
    : QObject (parent), mSectionName (section), mDefaultValue (defaultValue)
{
    if (sColumnNames.size() == 0)
        sColumnNames << "Setting Name" << "Section Name" << "Values"
                      << "Default Value" << "Value List" << "ProxyList"
                      << "Input Mask";

    setObjectName (name);
}

QVariant CSMSettings::Setting::item (int index) const
{
    switch (index)
    {
    case 0:     // setting name (read only)
        return name();
        break;
    case 1:     // section name (read only)
        return sectionName();
        break;
    case 2:     // setting value
        return values();
        break;
    case 3:     // setting default value (read only)
        return defaultValue();
        break;
    case 4:     // setting value list (read only)
        return valueList();
        break;
    case 5:     // setting proxy list (read only)
        return proxyList();
        break;
    case 6:     // setting input mask (read only)
        return inputMask();
        break;
    default:
        break;
    }

    return 0;
}

void CSMSettings::Setting::setValue (const QString &value, int index)
{
    //if index exceeds the size of the list, append to the end of the list
    if (index < mValues.size())
        mValues.replace (index,value);
    else
        mValues.append(value);
}

void CSMSettings::Setting::setItem (int index, QVariant value)
{
    switch (index)
    {
    case 1:
        setName (value.toString());
        break;
    case 2:
        setSection (value.toString());
        break;
    case 3:
        setValues (value.toStringList());
        break;
    case 4:
        setDeftaultValue (value.toString());
        break;
    case 5:
        setValueList (value.toStringList());
        break;
    case 6:
        setInputMask (value.toString());
        break;

    default:
        break;
    }
}
