#ifndef SETTING_SUPPORT_HPP
#define SETTING_SUPPORT_HPP

#include <Qt>
#include <QPair>
#include <QList>
#include <QVariant>
#include <QStringList>

//Enums
namespace CSMSettings
{
    ///Enumerated properties for scripting
    enum SettingProperty
    {
        Property_Name = 0,
        Property_Page = 1,
        Property_SettingType = 2,
        Property_IsMultiValue = 3,
        Property_IsMultiLine = 4,
        Property_WidgetWidth = 5,
        Property_ViewRow = 6,
        Property_ViewColumn = 7,
        Property_Delimiter = 8,
        Property_Serializable = 9,
        Property_ColumnSpan = 10,
        Property_RowSpan = 11,
        Property_Minimum = 12,
        Property_Maximum = 13,
        Property_SpecialValueText = 14,
        Property_Prefix = 15,
        Property_Suffix = 16,
        Property_SingleStep = 17,
        Property_Wrapping = 18,
        Property_TickInterval = 19,
        Property_TicksAbove = 20,
        Property_TicksBelow = 21,
        Property_StyleSheet = 22,
        Property_Label = 23,
        Property_ToolTip = 24,

        //Stringlists should always be the last items
        Property_DefaultValues = 25,
        Property_DeclaredValues = 26,
        Property_DefinedValues = 27,
        Property_Proxies = 28
    };

    ///Basic setting widget types.
    enum SettingType
    {
        /*
        * 0 - 9 - Boolean widgets
        * 10-19 - List widgets
        * 21-29 - Range widgets
        * 31-39 - Text widgets
        *
        * Each range corresponds to a View_Type enum by a factor of 10.
        *
        * Even-numbered values are single-value widgets
        * Odd-numbered values are multi-valued widgets
        */

        Type_CheckBox = 0,
        Type_RadioButton = 1,
        Type_ListView = 10,
        Type_ComboBox = 11,
        Type_SpinBox = 21,
        Type_DoubleSpinBox = 23,
        Type_Slider = 25,
        Type_Dial = 27,
        Type_TextArea = 30,
        Type_LineEdit = 31,
        Type_Undefined = 40
    };

}

namespace CSVSettings
{
    ///Categorical view types which encompass the setting widget types
    enum ViewType
    {
        ViewType_Boolean = 0,
        ViewType_List = 1,
        ViewType_Range = 2,
        ViewType_Text = 3,
        ViewType_Undefined = 4
    };
}


namespace CSMSettings
{
    ///used to construct default settings in the Setting class
    struct PropertyDefaultValues
    {
        int id;
        QString name;
        QVariant value;
    };

    ///strings which correspond to setting values.  These strings represent
    ///the script language keywords which would be used to declare setting
    ///views for 3rd party addons
    const QString sPropertyNames[] =
    {
        "name", "page", "setting_type", "is_multi_value",
        "is_multi_line", "widget_width", "view_row", "view_column", "delimiter",
        "is_serializable","column_span", "row_span", "minimum", "maximum",
        "special_value_text", "prefix", "suffix", "single_step", "wrapping",
        "tick_interval", "ticks_above", "ticks_below", "stylesheet",
        "defaults", "declarations", "definitions", "proxies"
    };

    ///Default values for a setting.  Used in setting creation.
    const QString sPropertyDefaults[] =
    {
        "",         //name
        "",         //page
        "40",       //setting type
        "false",    //multivalue
        "false",    //multiline
        "7",        //widget width
        "-1",       //view row
        "-1",       //view column
        ",",        //delimiter
        "true",     //serialized
        "1",        //column span
        "1",        //row span
        "0",        //value range
        "0",        //value minimum
        "0",        //value maximum
        "",         //special text
        "",         //prefix
        "",         //suffix
        "false",    //wrapping
        "1",        //tick interval
        "false",    //ticks above
        "true",     //ticks below
        "",         //StyleSheet
        "",         //default values
        "",         //declared values
        "",         //defined values
        ""          //proxy values
    };
}

#endif // VIEW_SUPPORT_HPP
