#include "apparatus.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadappa.hpp>

namespace MWLua
{
    void bindTES3Apparatus()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Apparatus>();

        usertypeDefinition.set("new", &ESM::Apparatus::blank);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Apparatus::mId);
        usertypeDefinition.set("mesh", &ESM::Apparatus::mModel);
        usertypeDefinition.set("icon", &ESM::Apparatus::mIcon);
        usertypeDefinition.set("script", &ESM::Apparatus::mScript);
        usertypeDefinition.set("name", &ESM::Apparatus::mName);

        usertypeDefinition.set("type", sol::property(
            [](ESM::Apparatus& self) { return self.mData.mType; },
            [](ESM::Apparatus& self, int value) { self.mData.mType = value; }
        ));
        usertypeDefinition.set("quality", sol::property(
            [](ESM::Apparatus& self) { return self.mData.mQuality; },
            [](ESM::Apparatus& self, float value) { self.mData.mQuality = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Apparatus& self) { return self.mData.mWeight; },
            [](ESM::Apparatus& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Apparatus& self) { return self.mData.mValue; },
            [](ESM::Apparatus& self, int value) { self.mData.mValue = value; }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3apparatus", usertypeDefinition);
    }
}
