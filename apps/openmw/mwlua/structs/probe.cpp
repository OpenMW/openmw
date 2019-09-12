#include "probe.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadprob.hpp>

namespace MWLua
{
    void bindTES3Probe()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Probe>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Probe::mId);
        usertypeDefinition.set("mesh", &ESM::Probe::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Probe& self) { return self.mIcon; },
            [](ESM::Probe& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Probe::mScript);
        usertypeDefinition.set("name", &ESM::Probe::mName);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Probe::mId);

        usertypeDefinition.set("quality", sol::property(
            [](ESM::Probe& self) { return self.mData.mQuality; },
            [](ESM::Probe& self, float value) { self.mData.mQuality = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Probe& self) { return self.mData.mWeight; },
            [](ESM::Probe& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Probe& self) { return self.mData.mValue; },
            [](ESM::Probe& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("maxCondition", sol::property(
            [](ESM::Probe& self) { return self.mData.mUses; },
            [](ESM::Probe& self, int value) { self.mData.mUses = value; }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3probe", usertypeDefinition);
    }
}
