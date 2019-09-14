#include "container.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadcont.hpp>

namespace MWLua
{
    void bindTES3Container()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Container>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Container::mId);
        usertypeDefinition.set("mesh", &ESM::Container::mModel);
        usertypeDefinition.set("script", &ESM::Container::mScript);
		usertypeDefinition.set("capacity", &ESM::Container::mWeight);

        // Friendly access to flags.
        usertypeDefinition.set("organic", sol::property(
            [](ESM::Container& self) { return (self.mFlags & ESM::Container::Organic) != 0; },
            [](ESM::Container& self, bool set) { set ? self.mFlags |= ESM::Container::Organic : self.mFlags &= ~ESM::Container::Organic; }
        ));
        usertypeDefinition.set("respawns", sol::property(
            [](ESM::Container& self) { return (self.mFlags & ESM::Container::Respawn) != 0; },
            [](ESM::Container& self, bool set) { set ? self.mFlags |= ESM::Container::Respawn : self.mFlags &= ~ESM::Container::Respawn; }
        ));

         // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Container::mId);

        // Finish up our usertype.
        state.set_usertype("tes3container", usertypeDefinition);
    }
}
