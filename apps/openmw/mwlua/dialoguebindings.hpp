#ifndef MWLUA_DIALOGUEBINDINGS_H
#define MWLUA_DIALOGUEBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;
    sol::table initCoreDialogueBindings(const Context&);
}

#endif // MWLUA_DIALOGUEBINDINGS_H
