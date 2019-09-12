#include "lockpick.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadlock.hpp>

namespace MWLua
{
    void bindTES3Lockpick()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Lockpick>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Lockpick::mId);
        usertypeDefinition.set("mesh", &ESM::Lockpick::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Lockpick& self) { return self.mIcon; },
            [](ESM::Lockpick& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Lockpick::mScript);
        usertypeDefinition.set("name", &ESM::Lockpick::mName);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Lockpick::mId);

        usertypeDefinition.set("quality", sol::property(
            [](ESM::Lockpick& self) { return self.mData.mQuality; },
            [](ESM::Lockpick& self, float value) { self.mData.mQuality = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Lockpick& self) { return self.mData.mWeight; },
            [](ESM::Lockpick& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Lockpick& self) { return self.mData.mValue; },
            [](ESM::Lockpick& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("maxCondition", sol::property(
            [](ESM::Lockpick& self) { return self.mData.mUses; },
            [](ESM::Lockpick& self, int value) { self.mData.mUses = value; }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3lockpick", usertypeDefinition);
    }
}
