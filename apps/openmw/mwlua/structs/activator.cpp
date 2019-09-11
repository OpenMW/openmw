#include "activator.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadacti.hpp>

namespace MWLua
{
    void bindTES3Activator()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Activator>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Activator::mId);
        usertypeDefinition.set("mesh", &ESM::Activator::mModel);
        usertypeDefinition.set("script", &ESM::Activator::mScript);

         // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Activator::mId);

        // Finish up our usertype.
        state.set_usertype("tes3activator", usertypeDefinition);
    }
}
