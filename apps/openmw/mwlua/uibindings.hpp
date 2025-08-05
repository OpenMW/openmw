#ifndef MWLUA_UIBINDINGS_H
#define MWLUA_UIBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initUserInterfacePackage(const Context& context);
}

#endif // MWLUA_UIBINDINGS_H
