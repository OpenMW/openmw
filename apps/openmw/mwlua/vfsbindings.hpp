#ifndef MWLUA_VFSBINDINGS_H
#define MWLUA_VFSBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initVFSPackage(const Context& context);
}

#endif // MWLUA_VFSBINDINGS_H
