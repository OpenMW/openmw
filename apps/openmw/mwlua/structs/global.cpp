#include "global.hpp"

#include "../luamanager.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/esmstore.hpp"

namespace MWLua
{
    void bindTES3GlobalVariable()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Global>();
        usertypeDefinition.set("new", sol::no_constructor);

        usertypeDefinition.set("id", &ESM::Global::mId);
        usertypeDefinition.set("value", sol::property(
            [](ESM::Global& self)
            {
                return MWBase::Environment::get().getWorld()->getGlobalFloat (self.mId);
            },
            [](ESM::Global& self, float value)
            {
                MWBase::Environment::get().getWorld()->setGlobalFloat (self.mId, value);
            }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3globalVariable", usertypeDefinition);
    }
}
