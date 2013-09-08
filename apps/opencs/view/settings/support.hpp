#ifndef VIEW_SUPPORT_HPP
#define VIEW_SUPPORT_HPP

#include <QList>
#include <QStringList>

#include "../../model/settings/support.hpp"

namespace CSVSettings
{
    struct WidgetDef;
    class ItemBlock;
    class GroupBlock;
    struct GroupBlockDef;

    typedef QList<GroupBlockDef *>              GroupBlockDefList;
    typedef QList<GroupBlock *>                 GroupBlockList;
    typedef QList<ItemBlock *>                  ItemBlockList;
    typedef QList<QStringList *>                ProxyList;
    typedef QList<WidgetDef *>                  WidgetList;
    typedef QMap<QString, ItemBlock *>          ItemBlockMap;

    enum Orientation
    {
        Orient_Horizontal,
        Orient_Vertical
    };

    enum WidgetType
    {
        Widget_CheckBox,
        Widget_ComboBox,
        Widget_LineEdit,
        Widget_ListBox,
        Widget_RadioButton,
        Widget_SpinBox,
        Widget_Undefined
    };

    enum Alignment
    {
        Align_Left    = Qt::AlignLeft,
        Align_Center  = Qt::AlignHCenter,
        Align_Right   = Qt::AlignRight
    };

    /// definition struct for widgets
    struct WidgetDef
    {
        /// type of widget providing input
        WidgetType type;

        /// width of caption label
        int labelWidth;

        /// width of input widget
        int widgetWidth;

        /// label / widget orientation (horizontal / vertical)
        Orientation orientation;

        /// input mask (line edit only)
        QString inputMask;

        /// label caption.  Leave empty for multiple items.  See BlockDef::captionList
        QString caption;

        /// widget value.   Leave empty for multiple items.  See BlockDef::valueList
        QString value;

        /// Min/Max QString value pair.  If empty, assigned to property item value pair.
        CSMSettings::QStringPair *minMax;

        /// value list for list widgets.  If left empty, is assigned to property item value list during block build().
        QStringList *valueList;

        /// determined at runtime
        bool isDefault;

        /// left / center / right-justify text in widget
        Alignment valueAlignment;

        /// left / center / right-justify widget in group box
        Alignment widgetAlignment;


        WidgetDef() :   labelWidth (-1), widgetWidth (-1),
                        orientation (Orient_Horizontal),
                        isDefault (true), valueAlignment (Align_Center),
                        widgetAlignment (Align_Right),
                        inputMask (""), value (""),
                        caption (""), valueList (0)
        {}

        WidgetDef (WidgetType widgType)
            : type (widgType), orientation (Orient_Horizontal),
              caption (""), value (""), valueAlignment (Align_Center),
              widgetAlignment (Align_Right),
              labelWidth (-1), widgetWidth (-1),
              valueList (0), isDefault (true)
        {}

    };

    /// Defines the attributes of the setting as it is represented in the config file
    /// as well as the UI elements (group box and widget) that serve it.
    /// Only one widget may serve as the input widget for the setting.
    struct SettingsItemDef
    {
        /// setting name
        QString name;

        /// list of valid values for the setting
        QStringList *valueList;

        /// Used to populate option widget captions or list widget item lists (see WidgetDef::caption / value)
        QString defaultValue;

        /// flag indicating multi-valued setting
        bool hasMultipleValues;

        /// minimum / maximum value pair
        CSMSettings::QStringPair minMax;

        /// definition of the input widget for this setting
        WidgetDef widget;

        /// general orientation of the widget / label for this setting
        Orientation orientation;

        /// list of settings and corresponding default values for proxy widget
        ProxyList *proxyList;

        SettingsItemDef() : name (""), defaultValue (""), orientation (Orient_Vertical), hasMultipleValues (false)
        {}

        SettingsItemDef (QString propName, QString propDefault, Orientation propOrient = Orient_Vertical)
            : name (propName), defaultValue (propDefault), orientation (propOrient),
              hasMultipleValues(false), valueList (new QStringList), proxyList ( new ProxyList)
        {}
    };


    /// Generic container block
    struct GroupBlockDef
    {
        /// block title
        QString title;

        /// list of captions for widgets at the block level (not associated with any particular setting)
        QStringList captions;

        /// list of widgets at the block level (not associated with any particular setting)
        WidgetList widgets;

        /// list of the settings which are subordinate to the setting block.
        QList<SettingsItemDef *> settingItems;

        /// general orientation of widgets in group block
        Orientation widgetOrientation;

        /// determines whether or not box border/title are visible
        bool isVisible;

        /// indicates whether or not this block defines a proxy block
        bool isProxy;

        /// generic default value attribute
        QString defaultValue;

        /// shows / hides margins
        bool isZeroMargin;

        GroupBlockDef (): title(""), widgetOrientation (Orient_Vertical), isVisible (true), isProxy (false), defaultValue (""), isZeroMargin (true)
        {}

        GroupBlockDef (QString blockTitle)
            : title (blockTitle), widgetOrientation (Orient_Vertical), isProxy (false), isVisible (true), defaultValue (""), isZeroMargin (true)
        {}
    };

    /// used to create unique, complex blocks
    struct CustomBlockDef
    {
        /// block title
        QString title;

        /// default value for widgets unique to the custom block
        QString defaultValue;

        /// list of settings groups that comprise the settings within the custom block
        GroupBlockDefList blockDefList;

        /// orientation of the widgets within the block
        Orientation blockOrientation;

        CustomBlockDef (): title (""), defaultValue (""), blockOrientation (Orient_Horizontal)
        {}

        CustomBlockDef (const QString &blockTitle)
            : title (blockTitle), defaultValue (""), blockOrientation (Orient_Horizontal)
        {}
    };
}

#endif // VIEW_SUPPORT_HPP
