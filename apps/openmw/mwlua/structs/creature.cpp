#include "alchemy.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadcrea.hpp>

namespace MWLua
{
    void bindTES3Creature()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Creature>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Creature::mId);

        // FIXME: AI structs bindings are not implemented yet
        //usertypeDefinition.set("aiConfig", sol::readonly_property(&TES3::Creature::aiConfig));
        usertypeDefinition.set("soul", sol::property(
            [](ESM::Creature& self) { return self.mData.mSoul; },
            [](ESM::Creature& self, int value) { self.mData.mSoul = value; }
        ));
        usertypeDefinition.set("soundCreature", &ESM::Creature::mOriginal);
        usertypeDefinition.set("spells", sol::readonly_property([](ESM::Creature& self) { return self.mSpells.mList; }));
        usertypeDefinition.set("type", sol::property(
            [](ESM::Creature& self) { return self.mData.mType; },
            [](ESM::Creature& self, int value) { self.mData.mType = value; }
        ));
        usertypeDefinition.set("bloodType", sol::property(
            [](ESM::Creature& self) { return self.mBloodType; },
            [](ESM::Creature& self, int value) { self.mBloodType = value; }
        ));
        usertypeDefinition.set("fatigue", sol::property(
            [](ESM::Creature& self) { return self.mData.mFatigue; },
            [](ESM::Creature& self, int value) { self.mData.mFatigue = value; }
        ));
        usertypeDefinition.set("gold", sol::property(
            [](ESM::Creature& self) { return self.mData.mGold; },
            [](ESM::Creature& self, int value) { self.mData.mGold = value; }
        ));
        usertypeDefinition.set("health", sol::property(
            [](ESM::Creature& self) { return self.mData.mHealth; },
            [](ESM::Creature& self, int value) { self.mData.mHealth = value; }
        ));
        usertypeDefinition.set("persistent", sol::readonly_property(&ESM::Creature::mPersistent));
        usertypeDefinition.set("level", sol::property(
            [](ESM::Creature& self) { return self.mData.mLevel; },
            [](ESM::Creature& self, int value) { self.mData.mLevel = value; }
        ));
        usertypeDefinition.set("magicka", sol::property(
            [](ESM::Creature& self) { return self.mData.mMana; },
            [](ESM::Creature& self, int value) { self.mData.mMana = value; }
        ));
        usertypeDefinition.set("mesh", &ESM::Creature::mModel);

        usertypeDefinition.set("name", &ESM::Creature::mName);
        usertypeDefinition.set("scale", sol::property(
            [](ESM::Creature& self) { return self.mScale; },
            [](ESM::Creature& self, float value) { self.mScale = value; }
        ));
        usertypeDefinition.set("script", &ESM::Creature::mScript);

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("attacks", sol::readonly_property([](ESM::Creature& self) { return std::ref(self.mData.mAttack); }));
        usertypeDefinition.set("attributes", sol::readonly_property([](ESM::Creature& self)
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            sol::table result = state.create_table();
            result[1] = self.mData.mStrength;
            result[2] = self.mData.mIntelligence;
            result[3] = self.mData.mWillpower;
            result[4] = self.mData.mAgility;
            result[5] = self.mData.mSpeed;
            result[6] = self.mData.mEndurance;
            result[7] = self.mData.mPersonality;
            result[8] = self.mData.mLuck;

            return result;
        }));
        usertypeDefinition.set("skills", sol::readonly_property([](ESM::Creature& self)
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            sol::table result = state.create_table();
            result[1] = self.mData.mCombat;
            result[2] = self.mData.mMagic;
            result[3] = self.mData.mStealth;

            return result;
        }));

        // Allow access to flags.
        usertypeDefinition.set("flags", sol::property(
            [](ESM::Creature& self) { return self.mFlags; },
            [](ESM::Creature& self, unsigned char value) { self.mFlags = value; }
        ));

        // Easy access to actor flags.
        usertypeDefinition.set("biped", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Bipedal) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Bipedal : self.mFlags &= ~ESM::Creature::Bipedal; }
        ));
        usertypeDefinition.set("respawns", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Respawn) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Respawn : self.mFlags &= ~ESM::Creature::Respawn; }
        ));
        usertypeDefinition.set("usesEquipment", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Weapon) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Weapon : self.mFlags &= ~ESM::Creature::Weapon; }
        ));
        usertypeDefinition.set("swims", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Swims) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Swims : self.mFlags &= ~ESM::Creature::Swims; }
        ));
        usertypeDefinition.set("flies", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Flies) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Flies : self.mFlags &= ~ESM::Creature::Flies; }
        ));
        usertypeDefinition.set("walks", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Walks) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Walks : self.mFlags &= ~ESM::Creature::Walks; }
        ));
        usertypeDefinition.set("essential", sol::property(
            [](ESM::Creature& self) { return (self.mFlags & ESM::Creature::Essential) != 0; },
            [](ESM::Creature& self, bool set) { set ? self.mFlags |= ESM::Creature::Essential : self.mFlags &= ~ESM::Creature::Essential; }
        ));

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Creature::mId);

        // Finish up our usertype.
        state.set_usertype("tes3creature", usertypeDefinition);
    }
}
