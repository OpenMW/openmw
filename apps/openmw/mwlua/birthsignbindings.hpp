#ifndef MWLUA_BIRTHSIGNBINDINGS_H
#define MWLUA_BIRTHSIGNBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initCoreBirthSignBindings(const Context& context);
}

#endif // MWLUA_BIRTHSIGNBINDINGS_H
