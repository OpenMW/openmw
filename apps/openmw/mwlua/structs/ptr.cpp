#include "ptr.hpp"

#include "../luamanager.hpp"

#include "../../mwworld/ptr.hpp"

namespace mwse {
	namespace lua {
		void bindTES3Reference() {
			// Get our lua state.
			auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
			sol::state& state = stateHandle.state;

			// Binding for MWWorld::Ptr
			{
				// Start our usertype. We must finish this with state.set_usertype.
				auto usertypeDefinition = state.create_simple_usertype<MWWorld::Ptr>();

				// Finish up our usertype.
				state.set_usertype("tes3reference", usertypeDefinition);
			}
		}
	}
}
