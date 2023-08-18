#ifndef MWLUA_MUSICBINDINGS_H
#define MWLUA_MUSICBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initMusicPackage(const Context&);
}

#endif // MWLUA_MUSICBINDINGS_H
