#ifndef MWLUA_MAGICBINDINGS_H
#define MWLUA_MAGICBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initCoreMagicBindings(const Context& context);
    void addActorMagicBindings(sol::table& actor, const Context& context);
}

#endif // MWLUA_MAGICBINDINGS_H
