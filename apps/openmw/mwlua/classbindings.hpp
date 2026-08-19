#ifndef MWLUA_CLASSBINDINGS_H
#define MWLUA_CLASSBINDINGS_H

#include <sol/forward.hpp>

namespace ESM
{
    struct Class;
}

namespace MWLua
{
    struct Context;

    sol::table initClassRecordBindings(const Context& context);
    void addMutableClassType(sol::state_view& lua);
    ESM::Class tableToClass(const sol::table& rec);
}

#endif // MWLUA_CLASSBINDINGS_H
