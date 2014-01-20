#ifndef SETTING_SUPPORT_HPP
#define SETTING_SUPPORT_HPP

#include <Qt>
#include <QPair>
#include <QList>

//Typedefs
namespace CSMSettings
{
    class Setting;

    // Definition / Declaration model typedefs
    // "Pair" = Setting name and specific data
    // "ListItem" = Page name and associated setting pair

    typedef QPair <QString, QString>                DefinitionPair;
    typedef QPair <QString, DefinitionPair>    DefinitionListItem;
    typedef QList <DefinitionListItem>              DefinitionList;

    typedef QPair <QString, Setting *> DeclarationPair;
    typedef QPair <QString, DeclarationPair> DeclarationListItem;
    typedef QList <DeclarationListItem> DeclarationList;
}

//Enums
namespace CSMSettings
{
    enum BooleanSettingColumn
    {
        BooleanSetting_Value = 0,
        BooleanSetting_ValueState = 1
    };

    enum SettingColumn
    {
        Setting_Name = 0,
        Setting_Page = 1,
        Setting_Value = 2,
        Setting_DefaultValue = 3,
        Setting_ValueList = 4,
        Setting_ViewType = 5,
        Setting_ValueCapacity = 6,
        Setting_Orientation = 7
    };

    enum MergeMethod
    {
        Merge_Accept,
        Merge_Ignore,
        Merge_Overwrite
    };
}

namespace CSVSettings
{
    enum ViewType
    {
        ViewType_Boolean,
        ViewType_List,
        ViewType_Range,
        ViewType_Text,
        ViewType_Undefined
    };

    enum Alignment
    {
        Align_Left    = Qt::AlignLeft,
        Align_Center  = Qt::AlignHCenter,
        Align_Right   = Qt::AlignRight
    };
}

#endif // VIEW_SUPPORT_HPP
