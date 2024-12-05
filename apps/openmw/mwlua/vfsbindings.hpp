#ifndef MWLUA_VFSBINDINGS_H
#define MWLUA_VFSBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initVFSPackage(const Context&);
}

#endif // MWLUA_VFSBINDINGS_H
