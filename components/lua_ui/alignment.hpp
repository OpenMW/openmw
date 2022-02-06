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

    MyGUI::Align alignmentToMyGui(Alignment horizontal, Alignment vertical);
}

#endif // !OPENMW_LUAUI_PROPERTIES
