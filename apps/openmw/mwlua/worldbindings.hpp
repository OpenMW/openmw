#ifndef MWLUA_WORLDBINDINGS_H
#define MWLUA_WORLDBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initWorldPackage(const Context&);
}

#endif // MWLUA_WORLDBINDINGS_H
