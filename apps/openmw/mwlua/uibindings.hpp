#ifndef MWLUA_UIBINDINGS_H
#define MWLUA_UIBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initUserInterfacePackage(const Context&);
}

#endif // MWLUA_UIBINDINGS_H
