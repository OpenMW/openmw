#ifndef MWLUA_ITEMDATA_H
#define MWLUA_ITEMDATA_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    void addItemDataBindings(sol::table& item, const Context& context);

}
#endif // MWLUA_ITEMDATA_H
