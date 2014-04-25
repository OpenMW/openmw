#ifndef SETTING_SUPPORT_HPP
#define SETTING_SUPPORT_HPP

#include <Qt>
#include <QPair>
#include <QList>
#include <QVariant>
#include <QStringList>

//Typedefs
namespace CSMSettings
{
    // Definition / Declaration model typedefs
    // "Pair" = Setting name and specific data
    // "ListItem" = Page name and associated setting pair

    typedef QPair <QString, QString> StringPair;
    typedef QPair <QString, QStringList> StringListPair;
    typedef QList <StringListPair> StringListPairs;

}

//Enums
namespace CSMSettings
{
    enum SettingProperty
    {
        Property_Name = 0,
        Property_Page = 1,
        Property_ViewType = 2,
        Property_IsMultiValue = 3,
        Property_IsMultiLine = 4,
        Property_WidgetWidth = 5,
        Property_ViewRow = 6,
        Property_ViewColumn = 7,
        Property_Delimiter = 8,
        Property_Serializable = 9,
        Property_ColumnSpan = 10,
        Property_RowSpan = 11,

        //Stringlists should always be the last items
        Property_DefaultValues = 12,
        Property_DeclaredValues = 13,
        Property_DefinedValues = 14,
        Property_Proxies = 15
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
        QVariant value;
    };

    const QString sPropertyNames[] =
    {
        "name", "page", "view_type", "is_multi_value",
        "is_multi_line", "widget_width", "view_row", "view_column", "delimiter",
        "is_serializable","column_span", "row_span",
        "defaults", "declarations", "definitions", "proxies"
    };

    const QString sPropertyDefaults[] =
    {
        "",         //name
        "",         //page
        "0",        //view type
        "false",    //multivalue
        "false",    //multiline
        "0",        //widget width
        "-1",       //view row
        "-1",       //view column
        ",",        //delimiter
        "true",     //serialized
        "1",        //column span
        "1",        //row span
        "",         //default values
        "",         //declared values
        "",         //defined values
        ""          //proxy values
    };
}

#endif // VIEW_SUPPORT_HPP
