#ifndef MWLUA_WORLDBINDINGS_H
#define MWLUA_WORLDBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initWorldPackage(const Context& context);
}

#endif // MWLUA_WORLDBINDINGS_H
