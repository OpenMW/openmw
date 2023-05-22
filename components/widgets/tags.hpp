#ifndef OPENMW_WIDGETS_TAGS_H
#define OPENMW_WIDGETS_TAGS_H

#include <MyGUI_UString.h>
#include <string_view>

namespace Gui
{

    /// Try to replace a tag. Returns true on success and writes the result to \a out.
    bool replaceTag(std::string_view tag, MyGUI::UString& out);

}

#endif
