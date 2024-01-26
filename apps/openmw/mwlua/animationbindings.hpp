#ifndef MWLUA_ANIMATIONBINDINGS_H
#define MWLUA_ANIMATIONBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    sol::table initAnimationPackage(const Context& context);
    sol::table initCoreVfxBindings(const Context& context);
}

#endif // MWLUA_ANIMATIONBINDINGS_H
