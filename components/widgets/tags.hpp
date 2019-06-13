#ifndef OPENMW_WIDGETS_TAGS_H
#define OPENMW_WIDGETS_TAGS_H

#include <MyGUI_UString.h>
#include <string>
#include <map>

namespace Gui
{

/// Try to replace a tag. Returns true on success and writes the result to \a out.
bool replaceTag (const MyGUI::UString& tag, MyGUI::UString& out);

}

#endif
