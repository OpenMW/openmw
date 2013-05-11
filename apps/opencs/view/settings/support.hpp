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

    enum OcsWidgetOrientation
    {
        OCS_HORIZONTAL,
        OCS_VERTICAL
    };

    enum OcsWidgetType
    {
        OCS_CHECK_WIDGET,
        OCS_COMBO_WIDGET,
        OCS_TEXT_WIDGET,
        OCS_LIST_WIDGET,
        OCS_RADIO_WIDGET,
        OCS_SPIN_WIDGET,
        OCS_UNDEFINED_WIDGET
    };

    enum OcsAlignment
    {
        OCS_LEFT    = Qt::AlignLeft,
        OCS_CENTER  = Qt::AlignHCenter,
        OCS_RIGHT   = Qt::AlignRight
    };

    //template for defining the widget of a property.
    struct WidgetDef
    {
        OcsWidgetType type;               //type of widget providing input
        int labelWidth;                   //width of caption label
        int widgetWidth;                  //width of input widget
        OcsWidgetOrientation orientation; //label / widget orientation (horizontal / vertical)
        QString inputMask;                //input mask (line edit)
        QString caption;                  //label caption.  Leave empty for multiple items.  See BlockDef::captionList
        QString value;                    //widget value.   Leave empty for multiple items.  See BlockDef::valueList
        CSMSettings::QStringPair *minMax; //Min/Max QString value pair.  If empty, assigned to property item value pair.
        QStringList *valueList;           //value list for list widgets.  If left empty, is assigned to property item value list during block build().
        bool isDefault;                   //isDefault - determined at runtime.
        OcsAlignment valueAlignment;      //left / center / right-justify text in widget
        OcsAlignment widgetAlignment;     //left / center / right-justify widget in group box


        WidgetDef() :   labelWidth (-1), widgetWidth (-1),
                        orientation (OCS_HORIZONTAL),
                        isDefault (true), valueAlignment (OCS_CENTER),
                        widgetAlignment (OCS_RIGHT),
                        inputMask (""), value (""),
                        caption (""), valueList (0)
        {}

        WidgetDef (OcsWidgetType widgType)
            : type (widgType), orientation (OCS_HORIZONTAL),
              caption (""), value (""), valueAlignment (OCS_CENTER),
              widgetAlignment (OCS_RIGHT),
              labelWidth (-1), widgetWidth (-1),
              valueList (0), isDefault (true)
        {}

    };

    //Defines the attributes of the property as it is represented in the config file
    //as well as the UI elements (group box and widget) that serve it.
    //Only one widget may serve as the input widget for the property.
    struct SettingsItemDef
    {
        QString name;                       //property name
        QStringList *valueList;             //list of valid values for the property.
                                            //Used to populate option widget captions or list widget item lists (see WidgetDef::caption / value)
        QString defaultValue;
        bool hasMultipleValues;
        CSMSettings::QStringPair minMax;    //minimum / maximum value pair
        WidgetDef widget;                   //definition of the input widget for this setting
        OcsWidgetOrientation orientation;   //general orientation of the widget / label for this property
        ProxyList *proxyList;               //list of property and corresponding default values for proxy widget

        SettingsItemDef() : name (""), defaultValue (""), orientation (OCS_VERTICAL), hasMultipleValues (false)
        {}

        SettingsItemDef (QString propName, QString propDefault, OcsWidgetOrientation propOrient = OCS_VERTICAL)
            : name (propName), defaultValue (propDefault), orientation (propOrient),
              hasMultipleValues(false), valueList (new QStringList), proxyList ( new ProxyList)
        {}
    };


    //Hierarchically, this is a "sub-section" of properties within a section, solely for UI organization.
    //Does not correlate to config file structure.
    struct GroupBlockDef
    {
        QString title;                          //title of the block containing the property or properties of this sub-section
        QStringList captions;                   //list of captions for widgets at the block level (not associated with any particular property)
        WidgetList widgets;                     //list of widgets at the block level (not associated with any particular property)
        QList<SettingsItemDef *> properties;    //list of the property(ies) which are subordinate to the property block.
        OcsWidgetOrientation widgetOrientation; //general orientation of widgets in group block
        bool isVisible;                         //determines whether or not box border/title are visible
        bool isProxy;                           //indicates whether or not this block defines a proxy block
        QString defaultValue;                   //generic default value attribute

        GroupBlockDef (): title(""), widgetOrientation (OCS_VERTICAL), isVisible (true), isProxy (false), defaultValue ("")
        {}

        GroupBlockDef (QString blockTitle)
            : title (blockTitle), widgetOrientation (OCS_VERTICAL), isProxy (false), isVisible (true), defaultValue ("")
        {}
    };

    struct CustomBlockDef
    {
        QString title;
        QString defaultValue;                   //default value for widgets unique to the custom block
        GroupBlockDefList blockDefList;         //list of settings groups that comprise the settings within the custom block
        OcsWidgetOrientation blockOrientation;

        CustomBlockDef (): title (""), defaultValue (""), blockOrientation (OCS_HORIZONTAL)
        {}

        CustomBlockDef (const QString &blockTitle)
            : title (blockTitle), defaultValue (""), blockOrientation (OCS_HORIZONTAL)
        {}
    };
}

#endif // VIEW_SUPPORT_HPP
