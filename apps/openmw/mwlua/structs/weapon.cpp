#include "weapon.hpp"

#include "../luamanager.hpp"

#include "../../mwmechanics/weapontype.hpp"

namespace MWLua
{
    void bindTES3Weapon()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Weapon>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Weapon::mId);
        usertypeDefinition.set("mesh", &ESM::Weapon::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Weapon& self) { return self.mIcon; },
            [](ESM::Weapon& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Weapon::mScript);
        usertypeDefinition.set("name", &ESM::Weapon::mName);
        usertypeDefinition.set("enchantment", &ESM::Weapon::mEnchant);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Weapon::mId);

        usertypeDefinition.set("type", sol::property(
            [](ESM::Weapon& self) { return self.mData.mType; },
            [](ESM::Weapon& self, int value) { self.mData.mType = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Weapon& self) { return self.mData.mWeight; },
            [](ESM::Weapon& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Weapon& self) { return self.mData.mValue; },
            [](ESM::Weapon& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("maxCondition", sol::property(
            [](ESM::Weapon& self) { return self.mData.mHealth; },
            [](ESM::Weapon& self, int value) { self.mData.mHealth = value; }
        ));
        usertypeDefinition.set("enchantCapacity", sol::property(
            [](ESM::Weapon& self) { return self.mData.mEnchant; },
            [](ESM::Weapon& self, int value) { self.mData.mEnchant = value; }
        ));
        usertypeDefinition.set("speed", sol::property(
            [](ESM::Weapon& self) { return self.mData.mSpeed; },
            [](ESM::Weapon& self, int value) { self.mData.mSpeed = value; }
        ));
        usertypeDefinition.set("reach", sol::property(
            [](ESM::Weapon& self) { return self.mData.mReach; },
            [](ESM::Weapon& self, int value) { self.mData.mReach = value; }
        ));
        usertypeDefinition.set("chopMin", sol::property(
            [](ESM::Weapon& self) { return self.mData.mChop[0]; },
            [](ESM::Weapon& self, int value) { self.mData.mChop[0] = value; }
        ));
        usertypeDefinition.set("chopMax", sol::property(
            [](ESM::Weapon& self) { return self.mData.mChop[1]; },
            [](ESM::Weapon& self, int value) { self.mData.mChop[1] = value; }
        ));
        usertypeDefinition.set("slashMin", sol::property(
            [](ESM::Weapon& self) { return self.mData.mSlash[0]; },
            [](ESM::Weapon& self, int value) { self.mData.mSlash[0] = value; }
        ));
        usertypeDefinition.set("slashMax", sol::property(
            [](ESM::Weapon& self) { return self.mData.mSlash[1]; },
            [](ESM::Weapon& self, int value) { self.mData.mSlash[1] = value; }
        ));
        usertypeDefinition.set("thrustMin", sol::property(
            [](ESM::Weapon& self) { return self.mData.mThrust[0]; },
            [](ESM::Weapon& self, int value) { self.mData.mThrust[0] = value; }
        ));
        usertypeDefinition.set("thrustMax", sol::property(
            [](ESM::Weapon& self) { return self.mData.mThrust[1]; },
            [](ESM::Weapon& self, int value) { self.mData.mThrust[1] = value; }
        ));

        // Allow access to flags.
        usertypeDefinition.set("flags", sol::property(
            [](ESM::Weapon& self) { return self.mData.mFlags; },
            [](ESM::Weapon& self, int value) { self.mData.mFlags = value; }
        ));

        // User-friendly access to those flags.
        usertypeDefinition.set("magical", sol::property(
            [](ESM::Weapon& self) { return (self.mData.mFlags & ESM::Weapon::Magical) != 0; },
            [](ESM::Weapon& self, bool set) { set ? self.mData.mFlags |= ESM::Weapon::Magical : self.mData.mFlags &= ~ESM::Weapon::Magical; }
        ));
        usertypeDefinition.set("silver", sol::property(
            [](ESM::Weapon& self) { return (self.mData.mFlags & ESM::Weapon::Silver) != 0; },
            [](ESM::Weapon& self, bool set) { set ? self.mData.mFlags |= ESM::Weapon::Silver : self.mData.mFlags &= ~ESM::Weapon::Silver; }
        ));

        // Functions exposed as properties.
        usertypeDefinition.set("hasDurability", sol::readonly_property([](ESM::Weapon& self) { return (MWMechanics::getWeaponType(self.mData.mType)->mFlags & ESM::WeaponType::HasHealth) != 0; }));
        usertypeDefinition.set("isOneHanded", sol::readonly_property([](ESM::Weapon& self) { return (MWMechanics::getWeaponType(self.mData.mType)->mFlags & ESM::WeaponType::TwoHanded) == 0; }));
        usertypeDefinition.set("isTwoHanded", sol::readonly_property([](ESM::Weapon& self) { return (MWMechanics::getWeaponType(self.mData.mType)->mFlags & ESM::WeaponType::TwoHanded) != 0; }));
        usertypeDefinition.set("isMelee", sol::readonly_property([](ESM::Weapon& self) { return MWMechanics::getWeaponType(self.mData.mType)->mWeaponClass == ESM::WeaponType::Melee; }));
        usertypeDefinition.set("isRanged", sol::readonly_property([](ESM::Weapon& self) { return MWMechanics::getWeaponType(self.mData.mType)->mWeaponClass == ESM::WeaponType::Ranged; }));
        usertypeDefinition.set("isThrown", sol::readonly_property([](ESM::Weapon& self) { return MWMechanics::getWeaponType(self.mData.mType)->mWeaponClass == ESM::WeaponType::Thrown; }));
        usertypeDefinition.set("isAmmo", sol::readonly_property([](ESM::Weapon& self) { return MWMechanics::getWeaponType(self.mData.mType)->mWeaponClass == ESM::WeaponType::Ammo; }));

        // Finish up our usertype.
        state.set_usertype("tes3weapon", usertypeDefinition);
    }
}
