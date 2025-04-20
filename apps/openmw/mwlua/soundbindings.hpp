#ifndef MWLUA_SOUNDBINDINGS_H
#define MWLUA_SOUNDBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initCoreSoundBindings(const Context& context);

    sol::table initAmbientPackage(const Context& context);
}

#endif // MWLUA_SOUNDBINDINGS_H
