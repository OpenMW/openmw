#ifndef MWLUA_CLASSBINDINGS_H
#define MWLUA_CLASSBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initClassRecordBindings(const Context& context);
}

#endif // MWLUA_CLASSBINDINGS_H
