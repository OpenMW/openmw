#ifndef MWLUA_RACEBINDINGS_H
#define MWLUA_RACEBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initRaceRecordBindings(const Context& context);
}

#endif // MWLUA_RACEBINDINGS_H
