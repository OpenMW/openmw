#include "ptr.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwworld/ptr.hpp"

#include <components/sceneutil/positionattitudetransform.hpp>

namespace MWLua
{
    void bindTES3Reference()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for MWWorld::Ptr
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<MWWorld::Ptr>();

            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set("sceneNode", sol::readonly_property([](MWWorld::Ptr& self) { return makeLuaNiPointer(self.getRefData().getBaseNode()->getChild(0)); }));

            // Finish up our usertype.
            state.set_usertype("tes3reference", usertypeDefinition);
        }
    }
}
