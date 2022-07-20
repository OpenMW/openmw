#include "alignment.hpp"

namespace LuaUi
{
    MyGUI::Align alignmentToMyGui(Alignment horizontal, Alignment vertical)
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
