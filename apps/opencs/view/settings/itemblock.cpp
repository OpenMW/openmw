#include "itemblock.hpp"

#include <QFontMetrics>

CSVSettings::ItemBlock::ItemBlock (QWidget* parent)
    : mSetting (0), AbstractBlock (false, parent)
{
}

int CSVSettings::ItemBlock::build(SettingsItemDef &iDef)
{
    buildItemBlock (iDef);
    buildItemBlockWidgets (iDef);

    return 0;
}

void CSVSettings::ItemBlock::buildItemBlockWidgets (SettingsItemDef &iDef)
{
    WidgetDef wDef = iDef.widget;
    QLayout *blockLayout = 0;
    QString defaultValue = iDef.defaultValue;

    switch (wDef.type)
    {

    case Widget_CheckBox:
    case Widget_RadioButton:

        foreach (QString item, *(iDef.valueList))
        {
            wDef.caption = item;
            wDef.isDefault = (item == defaultValue);

            blockLayout = buildWidget (item, wDef, blockLayout)->getLayout();
        }

    break;

    case Widget_ComboBox:
    case Widget_ListBox:

        //assign the item's value list to the widget's value list.
        //pass through to default to finish widget construction.
        if (!wDef.valueList)
            wDef.valueList = iDef.valueList;

    default:
        //only one instance of this non-list widget type.
        //Set it's value to the default value for the item and build the widget.

        if (wDef.value.isEmpty())
            wDef.value = iDef.defaultValue;

        buildWidget (iDef.name, wDef);
    }
}

void CSVSettings::ItemBlock::buildItemBlock (SettingsItemDef &iDef)
{
    QString defaultValue = iDef.defaultValue;

    setObjectName(iDef.name);

    mSetting = new CSMSettings::SettingsItem (objectName(),
                                 iDef.hasMultipleValues, iDef.defaultValue,
                                 parent());

    if (iDef.valueList)
        mSetting->setValueList(iDef.valueList);

    if (!iDef.minMax.isEmpty())
        mSetting->setValuePair(iDef.minMax);
}


bool CSVSettings::ItemBlock::update (const QString &value)
{
    bool success = updateItem (value);

    if (success)
        signalUpdateWidget (value);

    return success;
}


bool CSVSettings::ItemBlock::updateItem (const QString &value)
{
    return mSetting->updateItem(value);
}


bool CSVSettings::ItemBlock::updateBySignal(const QString &name, const QString &value, bool &doEmit)
{
    bool success = (mSetting->getValue() != value);

    if (success)
        success = updateItem(value);

    return success;
}

CSMSettings::SettingList *CSVSettings::ItemBlock::getSettings ()
{
    CSMSettings::SettingList *list = new CSMSettings::SettingList();
    list->push_back(mSetting);

    return list;
}

QString CSVSettings::ItemBlock::getValue() const
{
    return mSetting->getValue();
}
