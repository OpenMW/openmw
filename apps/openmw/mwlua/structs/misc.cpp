#include "misc.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadmisc.hpp>

namespace MWLua
{
    void bindTES3Misc()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Miscellaneous>();
        usertypeDefinition.set("new", sol::no_constructor);

        usertypeDefinition.set("id", &ESM::Miscellaneous::mId);
        usertypeDefinition.set("mesh", &ESM::Miscellaneous::mModel);
        usertypeDefinition.set("name", &ESM::Miscellaneous::mName);

        // Basic property binding.
        usertypeDefinition.set("value", sol::property(
            [](ESM::Miscellaneous& self) { return self.mData.mValue; },
            [](ESM::Miscellaneous& self, unsigned char value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Miscellaneous& self) { return self.mData.mWeight; },
            [](ESM::Miscellaneous& self, unsigned char value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("script", sol::property(&ESM::Miscellaneous::mScript));

        // Functions exposed as properties.
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Miscellaneous& self) { return self.mIcon; },
            [](ESM::Miscellaneous& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("isKey", sol::property(
            [](ESM::Miscellaneous& self) { return self.mData.mIsKey > 0; },
            [](ESM::Miscellaneous& self, bool value)
            {
                self.mData.mIsKey = value;
            }
        ));
        usertypeDefinition.set("isSoulGem", sol::readonly_property(
            [](ESM::Miscellaneous& self)
            {
                std::string soulgemFilter = "misc_soulgem"; // no other way to check for soulgems? :/
                return (self.mId.size() >= soulgemFilter.size() && self.mId.substr(0, soulgemFilter.size()) == soulgemFilter);
            }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3misc", usertypeDefinition);
    }
}
