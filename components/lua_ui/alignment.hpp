#ifndef OPENMW_LUAUI_ALIGNMENT
#define OPENMW_LUAUI_ALIGNMENT

#include <MyGUI_Align.h>

namespace LuaUi
{
    enum class Alignment
    {
        Start = 0,
        Center = 1,
        End = 2
    };

    inline MyGUI::Align alignmentToMyGui(Alignment horizontal, Alignment vertical)
    {
        MyGUI::Align align(MyGUI::Align::Center);
        if (horizontal == Alignment::Start)
            align |= MyGUI::Align::Left;
        if (horizontal == Alignment::End)
            align |= MyGUI::Align::Right;
        if (vertical == Alignment::Start)
            align |= MyGUI::Align::Top;
        if (vertical == Alignment::End)
            align |= MyGUI::Align::Bottom;
        return align;
    }
}

#endif
