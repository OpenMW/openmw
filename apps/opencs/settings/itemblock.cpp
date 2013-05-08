#include "itemblock.hpp"

#include <QFontMetrics>

CsSettings::ItemBlock::ItemBlock (QWidget* parent)
    : mSetting (0), AbstractBlock (false, parent)
{
}

int CsSettings::ItemBlock::build(SettingsItemDef &iDef)
{
    buildItemBlock (iDef);
    buildItemBlockWidgets (iDef);

    return 0;
}

void CsSettings::ItemBlock::buildItemBlockWidgets (SettingsItemDef &iDef)
{
    WidgetDef wDef = iDef.widget;
    QLayout *blockLayout = 0;
    QString defaultValue = iDef.defaultValue;

    switch (wDef.type)
    {

    case OCS_CHECK_WIDGET:
    case OCS_RADIO_WIDGET:

        foreach (QString item, *(iDef.valueList))
        {
            wDef.caption = item;
            wDef.isDefault = (item == defaultValue);

            blockLayout = buildWidget (item, wDef, blockLayout)->getLayout();
        }

    break;

    case OCS_COMBO_WIDGET:
    case OCS_LIST_WIDGET:

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

void CsSettings::ItemBlock::buildItemBlock (SettingsItemDef &iDef)
{
    QString defaultValue = iDef.defaultValue;

    setObjectName(iDef.name);

    mSetting = new SettingsItem (objectName(),
                                 iDef.hasMultipleValues, iDef.defaultValue,
                                 parent());

    if (iDef.valueList)
        mSetting->setValueList(iDef.valueList);

    if (!iDef.minMax.isEmpty())
        mSetting->setValuePair(iDef.minMax);
}


bool CsSettings::ItemBlock::update (const QString &value)
{
    bool success = updateItem (value);

    if (success)
        signalUpdateWidget (value);

    return success;
}


bool CsSettings::ItemBlock::updateItem (const QString &value)
{
    return mSetting->updateItem(value);
}


bool CsSettings::ItemBlock::updateBySignal(const QString &name, const QString &value, bool &doEmit)
{
    bool success = (mSetting->getValue() != value);

    if (success)
        success = updateItem(value);

    return success;
}

CsSettings::SettingList *CsSettings::ItemBlock::getSettings ()
{
    SettingList *list = new SettingList();
    list->push_back(mSetting);

    return list;
}

QString CsSettings::ItemBlock::getValue() const
{
    return mSetting->getValue();
}
