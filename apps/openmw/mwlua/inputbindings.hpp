#ifndef MWLUA_INPUTBINDINGS_H
#define MWLUA_INPUTBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initInputPackage(const Context&);
}

#endif // MWLUA_INPUTBINDINGS_H
