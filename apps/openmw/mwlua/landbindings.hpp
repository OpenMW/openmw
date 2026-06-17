#ifndef MWLUA_LANDBINDINGS_H
#define MWLUA_LANDBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initCoreLandBindings(const Context& context);
}

#endif // MWLUA_LANDBINDINGS_H
