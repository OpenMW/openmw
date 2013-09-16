#ifndef VIEW_SUPPORT_HPP
#define VIEW_SUPPORT_HPP

#include <QList>
#include <QStringList>

#include "../../model/settings/support.hpp"

namespace CSVSettings
{
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
        Widget_ToggleButton,
        Widget_Undefined
    };

    enum Alignment
    {
        Align_Left    = Qt::AlignLeft,
        Align_Center  = Qt::AlignHCenter,
        Align_Right   = Qt::AlignRight
    };
}

#endif // VIEW_SUPPORT_HPP
