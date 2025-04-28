#ifndef MWLUA_LANDBINDINGS_H
#define MWLUA_LANDBINDINGS_H

#include "context.hpp"

namespace MWLua
{
    sol::table initCoreLandBindings(const Context& context);
}

#endif // MWLUA_LANDBINDINGS_H
