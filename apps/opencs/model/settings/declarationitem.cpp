#include "declarationitem.hpp"

CSMSettings::DeclarationItem::DeclarationItem() :
    QStandardItem(sDeclarationColumnCount)
{
    child(Setting_ValueList)->setData("ValueList");

    child(Setting_ViewType)->setData
                (QVariant(static_cast<int>(CSVSettings::ViewType_Undefined)));

    child (Setting_WidgetWidth)->   setData (QVariant (0));
    child (Setting_ViewRow)->       setData (QVariant (-1));
    child (Setting_ViewColumn)->    setData (QVariant (-1));
    child (Setting_IsHorizontal)->  setData (QVariant (true));
    child (Setting_IsMultiLine)->   setData (QVariant (false));
    child (Setting_IsMultiValue)->  setData (QVariant (false));
    child (Setting_Name)->          setData (QVariant (""));
    child (Setting_Page)->          setData (QVariant (""));
    child (Setting_DefaultValue)->  setData (QVariant (""));
}

bool CSMSettings::DeclarationItem::setProperty(SettingProperty prop,
                                               bool value)
{
    bool *ok = true;

    switch (prop)
    {
    case Setting_IsHorizontal:
    case Setting_IsMultiLine:
    case Setting_IsMultiValue:
        value.toBool(ok);
        if (!ok)
            return false;
    break;

    default:
        return false;
    break;
    }

    child (prop)->setData(value);

    return ok;
}

bool CSMSettings::DeclarationItem::setProperty(SettingProperty prop,
                                               int value)
{
    bool *ok = true;
    switch (prop)
    {
    case Setting_ViewType:
    case Setting_WidgetWidth:
    case Setting_ViewRow:
    case Setting_ViewColumn:
        value.toInt(ok);
        if (!ok)
            return false;
    break;

    default:
        return false;
    break;
    }

    child (prop)->setData(value);

    return ok;
}

bool CSMSettings::DeclarationItem::setProperty(SettingProperty prop,
                                               const QString &value)
{
    bool *ok = true;

    switch (prop)
    {
    case Setting_Name:
    case Setting_Page:
    case Setting_DefaultValue:
    break;

    default:
        return false;
    break;
    }

    child (prop)->setData(value);

    return ok;
}

void CSMSettings::DeclarationItem::setValueList(const QStringList list)
{
    foreach (const QString &value, list)
        child(Setting_ValueList)->appendRow(new QStandardItem(value));
}
