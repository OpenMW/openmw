#include "armor.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadarmo.hpp>

namespace MWLua
{
    void bindTES3Armor()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Armor>();

        usertypeDefinition.set("new", &ESM::Armor::blank);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Armor::mId);
        usertypeDefinition.set("mesh", &ESM::Armor::mModel);
        usertypeDefinition.set("icon", &ESM::Armor::mIcon);
        usertypeDefinition.set("script", &ESM::Armor::mScript);
        usertypeDefinition.set("name", &ESM::Armor::mName);
        usertypeDefinition.set("enchantment", &ESM::Armor::mEnchant);

        usertypeDefinition.set("slot", sol::property(
            [](ESM::Armor& self) { return self.mData.mType; },
            [](ESM::Armor& self, int value) { self.mData.mType = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Armor& self) { return self.mData.mWeight; },
            [](ESM::Armor& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Armor& self) { return self.mData.mValue; },
            [](ESM::Armor& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("maxCondition", sol::property(
            [](ESM::Armor& self) { return self.mData.mHealth; },
            [](ESM::Armor& self, int value) { self.mData.mHealth = value; }
        ));
         usertypeDefinition.set("enchantCapacity", sol::property(
            [](ESM::Armor& self) { return self.mData.mEnchant; },
            [](ESM::Armor& self, int value) { self.mData.mEnchant = value; }
        ));
        usertypeDefinition.set("armorRating", sol::property(
            [](ESM::Armor& self) { return self.mData.mEnchant; },
            [](ESM::Armor& self, int value) { self.mData.mEnchant = value; }
        ));

        // FIXME: bodyparts are not mapped yet
        // PartReferenceList mParts;
        //usertypeDefinition.set("parts", sol::readonly_property([](TES3::Armor& self) { return std::ref(self.parts); }));

        //usertypeDefinition.set("isLeftPart", sol::property(&TES3::Armor::isLeftPartOfPair));


        // FIXME: can not be here since requires references
        //usertypeDefinition.set("slotName", sol::property(&TES3::Armor::getTypeName));
        //usertypeDefinition.set("weightClass", sol::property(&TES3::Armor::getWeightClass));

        /*
        // Basic function binding.
        usertypeDefinition.set("calculateArmorRating", [](ESM::Armor& self, sol::object actor)
        {
        });
        */

        // Finish up our usertype.
        state.set_usertype("tes3armor", usertypeDefinition);
    }
}
