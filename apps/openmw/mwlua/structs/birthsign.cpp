#include "birthsign.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadbsgn.hpp>

namespace MWLua
{
    void bindTES3BirthSign()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for TES3::BirthSign.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::BirthSign>();
            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set("id", sol::readonly_property([](ESM::BirthSign& self) { return self.mId; }));
            usertypeDefinition.set("description", &ESM::BirthSign::mDescription);
            usertypeDefinition.set("name", &ESM::BirthSign::mName);
            usertypeDefinition.set("texture", &ESM::BirthSign::mTexture);

            // Indirect bindings to unions and arrays.
            usertypeDefinition.set("powers", sol::readonly_property([](ESM::BirthSign& self) { return self.mPowers.mList; }));

            // Finish up our usertype.
            state.set_usertype("tes3birthSign", usertypeDefinition);
        }
    }
}
