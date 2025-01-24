#ifndef MWLUA_WEATHERBINDINGS_H
#define MWLUA_WEATHERBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{

    sol::table initCoreWeatherBindings(const Context&);

}

#endif // MWLUA_WEATHERBINDINGS_H
