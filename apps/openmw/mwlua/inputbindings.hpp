#ifndef MWLUA_INPUTBINDINGS_H
#define MWLUA_INPUTBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initInputPackage(const Context& context);
}

#endif // MWLUA_INPUTBINDINGS_H
