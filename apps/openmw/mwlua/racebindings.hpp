#ifndef MWLUA_RACEBINDINGS_H
#define MWLUA_RACEBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initRaceRecordBindings(const Context& context);
}

#endif // MWLUA_RACEBINDINGS_H
