#ifndef MWLUA_CONTENTBINDINGS_H
#define MWLUA_CONTENTBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initContentPackage(const Context& context);
}

#endif // MWLUA_CONTENTBINDINGS_H
