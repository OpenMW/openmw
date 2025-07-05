#ifndef MWLUA_REGIONBINDINGS_H
#define MWLUA_REGIONBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initCoreRegionBindings(const Context& context);
}

#endif // MWLUA_REGIONBINDINGS_H
