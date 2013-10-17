#include "setting.hpp"
#include <QStringList>

int CSMSettings::Setting::sColumnCount = 9;
QStringList CSMSettings::Setting::sColumnNames;

CSMSettings::Setting::Setting (const QString &name, const QString &section,
                               const QString &defaultValue, QObject *parent)
    : QObject (parent), mSectionName (section), mDefaultValue (defaultValue),
      mIsHorizontal (true)
{
    if (sColumnNames.size() == 0)
        sColumnNames << "Setting Name" << "Section Name" << "Values"
                      << "Default Value" << "Value List" << "ProxyList"
                      << "Input Mask" << "Widget Type" << "IsHorizontal";

    setObjectName (name);
}

QVariant CSMSettings::Setting::item (int index) const
{
    switch (index)
    {
    case 0:     // setting name
        return name();
        break;

    case 1:     // section name
        return sectionName();
        break;

    case 2:     // setting value
        return values();
        break;

    case 3:     // setting default value
        return defaultValue();
        break;

    case 4:     // setting value list
        return valueList();
        break;

    case 5:     // setting proxy list
        //return mProxyMap; //proxyMap();
        break;

    case 6:     // setting input mask
        return inputMask();
        break;

    case 7:     // setting widget type
        return widgetType();
        break;

    case 8:     // setting orientation (simgle-valued settings with value lists)
        return isHorizontal();
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
    case 0:
        setName         (value.toString());
        break;

    case 1:
        setSectionName  (value.toString());
        break;

    case 2:
        setValues       (value.toStringList());
        break;

    case 3:
        setDefaultValue (value.toString());
        break;

    case 4:
        setValueList    (value.toStringList());
        break;

    case 5:
        //setProxyMap     (static_cast<ProxyMap>(value.toMap()));
        break;

    case 6:
        setInputMask    (value.toString());
        break;

   case 7:
        setWidgetType   (static_cast<CSVSettings::WidgetType> (value.toInt()));

    default:
        break;
    }
}
