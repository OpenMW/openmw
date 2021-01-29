#include "luabindings.hpp"

#include "luamanagerimp.hpp"

namespace MWLua
{

    sol::table initUserInterfacePackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        api["showMessage"] = [luaManager=context.mLuaManager](std::string_view message)
        {
            luaManager->addUIMessage(message);
        };
        return context.mLua->makeReadOnly(api);
    }
    
}
