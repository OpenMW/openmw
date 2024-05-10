#ifndef MWLUA_DIALOGUEBINDINGS_H
#define MWLUA_DIALOGUEBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initCoreDialogueBindings(const Context& context);
}

#endif // MWLUA_DIALOGUEBINDINGS_H
