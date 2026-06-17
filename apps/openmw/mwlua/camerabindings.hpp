#ifndef MWLUA_CAMERABINDINGS_H
#define MWLUA_CAMERABINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    sol::table initCameraPackage(sol::state_view lua);
}

#endif // MWLUA_CAMERABINDINGS_H
