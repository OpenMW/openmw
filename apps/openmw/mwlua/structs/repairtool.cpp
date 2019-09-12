#include "repairtool.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadrepa.hpp>

namespace MWLua
{
    void bindTES3RepairTool()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Repair>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Repair::mId);
        usertypeDefinition.set("mesh", &ESM::Repair::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Repair& self) { return self.mIcon; },
            [](ESM::Repair& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Repair::mScript);
        usertypeDefinition.set("name", &ESM::Repair::mName);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Repair::mId);

        usertypeDefinition.set("quality", sol::property(
            [](ESM::Repair& self) { return self.mData.mQuality; },
            [](ESM::Repair& self, float value) { self.mData.mQuality = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Repair& self) { return self.mData.mWeight; },
            [](ESM::Repair& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Repair& self) { return self.mData.mValue; },
            [](ESM::Repair& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("maxCondition", sol::property(
            [](ESM::Repair& self) { return self.mData.mUses; },
            [](ESM::Repair& self, int value) { self.mData.mUses = value; }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3repairTool", usertypeDefinition);
    }
}
