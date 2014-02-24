#ifndef SETTING_SUPPORT_HPP
#define SETTING_SUPPORT_HPP

#include <Qt>
#include <QPair>
#include <QList>
#include <QVariant>

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

    static int sSettingPropertyCount = 11;
    static int sSettingPropertyListCount = 3;
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
        Property_Delimiter = 10
    };

    //PropertyList_DefinedValues needs to always be the last item in the list.
    //New declaration lists should be inserted before it and the integer values
    //reassigned accordingly
    enum SettingPropertyList
    {
        PropertyList_DeclaredValues = 0,
        PropertyList_DefinedValues = 1,
        PropertyList_Proxies = 2
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
        ViewType_Boolean = 0,
        ViewType_List = 1,
        ViewType_Range = 2,
        ViewType_Text = 3,
        ViewType_Undefined = 4
    };

    enum Alignment
    {
        Align_Left    = Qt::AlignLeft,
        Align_Center  = Qt::AlignHCenter,
        Align_Right   = Qt::AlignRight
    };
}

//
namespace CSMSettings
{
    struct PropertyDefaultValues
    {
        int id;
        QString name;
        QString value;
    };

    const PropertyDefaultValues sPropertyDefaults[] =
    {
        {Property_Name, "name", "undefined"},
        {Property_Page, "page", "undefined"},
        {Property_DefaultValue, "default", "undefined"},
        {Property_ViewType, "view type",
                QVariant (static_cast<int>
                                (CSVSettings::ViewType_Undefined)).toString() },
        {Property_IsMultiValue, "multi-value", QVariant (false).toString() },
        {Property_IsHorizontal, "horizontal", QVariant (true).toString() },
        {Property_IsMultiLine, "multi-line", QVariant (false).toString() },
        {Property_WidgetWidth, "width", QVariant (0).toString() },
        {Property_ViewRow, "view row", QVariant (-1).toString() },
        {Property_ViewColumn, "view column", QVariant (-1).toString() },
        {Property_Delimiter, "delimiter", "" }
    };

    const PropertyDefaultValues sPropertyListDefaults[] =
    {
        {PropertyList_DeclaredValues, "declarations", ""},
        {PropertyList_DefinedValues, "definitions", ""},
        {PropertyList_Proxies, "proxies", ""}
    };
}

#endif // VIEW_SUPPORT_HPP
