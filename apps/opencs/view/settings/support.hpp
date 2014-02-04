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

    int sSettingPropertyCount = 10;
    int sSettingPropertyListCount = 3;
}

//Enums
namespace CSMSettings
{
    enum BooleanSettingProperty
    {
        BooleanProperty_Value = 0,
        BooleanProperty_ValueState = 1
    };

    enum SettingProperty
    {
        Property_Name = 0,
        Property_Page = 1,
        Property_DefaultValue = 2,
        Property_ViewType = 3,
        Property_IsMultiValue = 4,
        Property_IsHorizontal = 5,
        Property_IsMultiLine = 6,
        Property_WidgetWidth = 7,
        Property_ViewRow = 8,
        Property_ViewColumn = 9,
    };

    enum SettingPropertyList
    {
        PropertyList_DefinedValues = 0,
        PropertyList_DeclaredValues = 1,
        PropertyList_Proxy = 2
    };

    enum SettingType
    {
        Type_MultiBool = 0,
        Type_SingleBool = 1,
        Type_MultiList = 2,
        Type_SingleList  = 3,
        Type_MultiRange = 4,
        Type_SingleRange = 5,
        Type_MultiText = 6,
        Type_SingleText = 7
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
