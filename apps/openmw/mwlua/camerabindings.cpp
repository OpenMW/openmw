#include "luabindings.hpp"

namespace MWLua
{

    sol::table initCameraPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        // TODO
        return LuaUtil::makeReadOnly(api);
    }

}
