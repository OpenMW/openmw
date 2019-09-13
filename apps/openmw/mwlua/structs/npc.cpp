#include "npc.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadnpc.hpp>

#include <components/misc/stringops.hpp>

namespace MWLua
{
    void bindTES3NPC()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::NPC>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::NPC::mId);

        usertypeDefinition.set("baseDisposition", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mDisposition; },
            [](ESM::NPC& self, int value) { self.mNpdt.mDisposition = value; }
        ));
        usertypeDefinition.set("bloodType", sol::property(
            [](ESM::NPC& self) { return self.mBloodType; },
            [](ESM::NPC& self, int value) { self.mBloodType = value; }
        ));
        usertypeDefinition.set("class", &ESM::NPC::mClass);
        usertypeDefinition.set("hair", &ESM::NPC::mHair);
        usertypeDefinition.set("head", &ESM::NPC::mHead);
        usertypeDefinition.set("faction", &ESM::NPC::mFaction);
        usertypeDefinition.set("factionIndex", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mFactionID; },
            [](ESM::NPC& self, char value) { self.mNpdt.mFactionID = value; }
        ));

        // FIXME: AI structs bindings are not implemented yet
        //usertypeDefinition.set("aiConfig", sol::readonly_property(&TES3::NPC::aiConfig));
        usertypeDefinition.set("spells", sol::readonly_property([](ESM::NPC& self) { return self.mSpells.mList; }));
        usertypeDefinition.set("fatigue", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mFatigue; },
            [](ESM::NPC& self, int value) { self.mNpdt.mFatigue = value; }
        ));
        usertypeDefinition.set("gold", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mGold; },
            [](ESM::NPC& self, int value) { self.mNpdt.mGold = value; }
        ));
        usertypeDefinition.set("factionRank", sol::property(
            [](ESM::NPC& self) { return self.getFactionRank(); },
            [](ESM::NPC& self, unsigned char value) { self.mNpdt.mRank = value; }
        ));
        usertypeDefinition.set("health", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mHealth; },
            [](ESM::NPC& self, int value) { self.mNpdt.mHealth = value; }
        ));
        usertypeDefinition.set("persistent", sol::readonly_property(&ESM::NPC::mPersistent));
        usertypeDefinition.set("level", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mLevel; },
            [](ESM::NPC& self, int value) { self.mNpdt.mLevel = value; }
        ));
        usertypeDefinition.set("reputation", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mReputation; },
            [](ESM::NPC& self, int value) { self.mNpdt.mReputation = value; }
        ));
        usertypeDefinition.set("magicka", sol::property(
            [](ESM::NPC& self) { return self.mNpdt.mMana; },
            [](ESM::NPC& self, int value) { self.mNpdt.mMana = value; }
        ));
        usertypeDefinition.set("mesh", &ESM::NPC::mModel);

        usertypeDefinition.set("name", &ESM::NPC::mName);
        usertypeDefinition.set("race", &ESM::NPC::mRace);
        usertypeDefinition.set("script", &ESM::NPC::mScript);

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("attributes", sol::readonly_property([](ESM::NPC& self)
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            sol::table result = state.create_table();
            result[1] = self.mNpdt.mStrength;
            result[2] = self.mNpdt.mIntelligence;
            result[3] = self.mNpdt.mWillpower;
            result[4] = self.mNpdt.mAgility;
            result[5] = self.mNpdt.mSpeed;
            result[6] = self.mNpdt.mEndurance;
            result[7] = self.mNpdt.mPersonality;
            result[8] = self.mNpdt.mLuck;

            return result;
        }));
        usertypeDefinition.set("skills", sol::property([](ESM::NPC& self) { return std::ref(self.mNpdt.mSkills); }));

        // Allow access to flags.
        usertypeDefinition.set("name", &ESM::NPC::mFlags);

        // Easy access to actor flags.
        usertypeDefinition.set("female", sol::property(
            [](ESM::NPC& self) { return (self.mFlags & ESM::NPC::Female) != 0; },
            [](ESM::NPC& self, bool set) { set ? self.mFlags |= ESM::NPC::Female : self.mFlags &= ~ESM::NPC::Female; }
        ));
        usertypeDefinition.set("essential", sol::property(
            [](ESM::NPC& self) { return (self.mFlags & ESM::NPC::Essential) != 0; },
            [](ESM::NPC& self, bool set) { set ? self.mFlags |= ESM::NPC::Essential : self.mFlags &= ~ESM::NPC::Essential; }
        ));
        usertypeDefinition.set("respawns", sol::property(
            [](ESM::NPC& self) { return (self.mFlags & ESM::NPC::Respawn) != 0; },
            [](ESM::NPC& self, bool set) { set ? self.mFlags |= ESM::NPC::Respawn : self.mFlags &= ~ESM::NPC::Respawn; }
        ));
        usertypeDefinition.set("autoCalc", sol::property(
            [](ESM::NPC& self) { return (self.mFlags & ESM::NPC::Autocalc) != 0; },
            [](ESM::NPC& self, bool set) { set ? self.mFlags |= ESM::NPC::Autocalc : self.mFlags &= ~ESM::NPC::Autocalc; }
        ));

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::NPC::mId);

        // Functions exposed as properties.
        usertypeDefinition.set("isGuard", sol::readonly_property(
            [](ESM::NPC& self) { return Misc::StringUtils::ciEqual(self.mClass, "guard"); }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3npc", usertypeDefinition);
    }
}
