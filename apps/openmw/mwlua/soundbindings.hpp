#ifndef MWLUA_SOUNDBINDINGS_H
#define MWLUA_SOUNDBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initCoreSoundBindings(const Context&);

    sol::table initAmbientPackage(const Context& context);
}

#endif // MWLUA_SOUNDBINDINGS_H
