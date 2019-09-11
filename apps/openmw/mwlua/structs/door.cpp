#include "door.hpp"

#include "../luamanager.hpp"

#include <components/esm/loaddoor.hpp>

namespace MWLua
{
    void bindTES3Door()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Door>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Door::mId);
        usertypeDefinition.set("mesh", &ESM::Door::mModel);
        usertypeDefinition.set("script", &ESM::Door::mScript);
        usertypeDefinition.set("name", &ESM::Door::mName);
        usertypeDefinition.set("openSound", &ESM::Door::mOpenSound);
        usertypeDefinition.set("closeSound", &ESM::Door::mCloseSound);

        // Finish up our usertype.
        state.set_usertype("tes3door", usertypeDefinition);
    }
}
