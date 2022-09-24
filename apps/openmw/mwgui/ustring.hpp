#ifndef MWGUI_USTRING_H
#define MWGUI_USTRING_H

#include <MyGUI_UString.h>

namespace MWGui
{
    // FIXME: Remove once we get a version of MyGUI that supports string_view
    inline MyGUI::UString toUString(std::string_view string)
    {
        return { string.data(), string.size() };
    }
}

#endif