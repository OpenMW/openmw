#include "clothing.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadclot.hpp>

namespace MWLua
{
    void bindTES3Clothing()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Clothing>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Clothing::mId);
        usertypeDefinition.set("mesh", &ESM::Clothing::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Clothing& self) { return self.mIcon; },
            [](ESM::Clothing& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Clothing::mScript);
        usertypeDefinition.set("name", &ESM::Clothing::mName);
        usertypeDefinition.set("enchantment", &ESM::Clothing::mEnchant);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Clothing::mId);

        usertypeDefinition.set("slot", sol::property(
            [](ESM::Clothing& self) { return self.mData.mType; },
            [](ESM::Clothing& self, int value) { self.mData.mType = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Clothing& self) { return self.mData.mWeight; },
            [](ESM::Clothing& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Clothing& self) { return self.mData.mValue; },
            [](ESM::Clothing& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("enchantCapacity", sol::property(
            [](ESM::Clothing& self) { return self.mData.mEnchant; },
            [](ESM::Clothing& self, int value) { self.mData.mEnchant = value; }
        ));

        // FIXME: bodyparts are not mapped yet
        // PartReferenceList mParts;
        //usertypeDefinition.set("parts", sol::readonly_property([](TES3::Armor& self) { return std::ref(self.parts); }));

        //usertypeDefinition.set("isLeftPart", sol::property(&TES3::Armor::isLeftPartOfPair));

        // FIXME: can not be here since requires references
        //usertypeDefinition.set("slotName", sol::property(&TES3::Armor::getTypeName));

        // Finish up our usertype.
        state.set_usertype("tes3armor", usertypeDefinition);
    }
}
