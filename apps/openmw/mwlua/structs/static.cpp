#include "static.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadstat.hpp>

namespace MWLua
{
    void bindTES3Static()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Static>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Static::mId);
        usertypeDefinition.set("mesh", &ESM::Static::mModel);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Static::mId);

        // Finish up our usertype.
        state.set_usertype("tes3static", usertypeDefinition);
    }
}
