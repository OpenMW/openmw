#include "static.hpp"

#include "../luamanager.hpp"

#include "../../mwworld/esmstore.hpp"

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

        usertypeDefinition.set("new", &ESM::Static::blank);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Static::mId);
        usertypeDefinition.set("mesh", &ESM::Static::mModel);

        // Finish up our usertype.
        state.set_usertype("tes3static", usertypeDefinition);
    }
}
