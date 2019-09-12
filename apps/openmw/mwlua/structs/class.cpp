#include "light.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadclas.hpp>

namespace MWLua
{
    void bindTES3Class()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Class>();

        usertypeDefinition.set("new", sol::constructors<ESM::Class()>());

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Class::mId);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Class& self) { return self.mName; },
            [](ESM::Class& self, const char* value) { if (strlen(value) < 32) self.mName = value; }
        ));
        usertypeDefinition.set("description", sol::property(
            [](ESM::Class& self) { return self.mDescription; },
            [](ESM::Class& self, int value) { self.mDescription = value; }
        ));
        usertypeDefinition.set("specialization", sol::property(
            [](ESM::Class& self) { return self.mData.mSpecialization; },
            [](ESM::Class& self, int value) { self.mData.mSpecialization = value; }
        ));
        usertypeDefinition.set("playable", sol::property(
            [](ESM::Class& self) { return self.mData.mIsPlayable > 0; },
            [](ESM::Class& self, bool value) { self.mData.mIsPlayable = value; }
        ));
        usertypeDefinition.set("services", sol::property(
            [](ESM::Class& self) { return self.mData.mCalc; },
            [](ESM::Class& self, int value) { self.mData.mCalc = value; }
        ));

        // User-friendly access to services
        usertypeDefinition.set("bartersWeapons", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Weapon) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Weapon : self.mData.mCalc &= ~ESM::Class::Weapon; }
        ));
        usertypeDefinition.set("bartersArmor", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Armor) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Armor : self.mData.mCalc &= ~ESM::Class::Armor; }
        ));
        usertypeDefinition.set("bartersClothing", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Clothing) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Clothing : self.mData.mCalc &= ~ESM::Class::Clothing; }
        ));
        usertypeDefinition.set("bartersBooks", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Books) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Books : self.mData.mCalc &= ~ESM::Class::Books; }
        ));
        usertypeDefinition.set("bartersIngredients", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Ingredient) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Ingredient : self.mData.mCalc &= ~ESM::Class::Ingredient; }
        ));
        usertypeDefinition.set("bartersLockpicks", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Lockpick) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Lockpick : self.mData.mCalc &= ~ESM::Class::Lockpick; }
        ));
        usertypeDefinition.set("bartersProbes", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Probe) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Probe : self.mData.mCalc &= ~ESM::Class::Probe; }
        ));
        usertypeDefinition.set("bartersLights", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Lights) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Lights : self.mData.mCalc &= ~ESM::Class::Lights; }
        ));
        usertypeDefinition.set("bartersApparatus", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Apparatus) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Apparatus : self.mData.mCalc &= ~ESM::Class::Apparatus; }
        ));
        usertypeDefinition.set("bartersRepairTools", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Repair) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Repair : self.mData.mCalc &= ~ESM::Class::Repair; }
        ));
        usertypeDefinition.set("bartersMiscItems", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Misc) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Misc : self.mData.mCalc &= ~ESM::Class::Misc; }
        ));
        usertypeDefinition.set("offersSpells", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Spells) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Spells : self.mData.mCalc &= ~ESM::Class::Spells; }
        ));
         usertypeDefinition.set("bartersEnchantedItems", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::MagicItems) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::MagicItems : self.mData.mCalc &= ~ESM::Class::MagicItems; }
        ));
        usertypeDefinition.set("bartersAlchemy", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Potions) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Potions : self.mData.mCalc &= ~ESM::Class::Potions; }
        ));
        usertypeDefinition.set("offersTraining", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Training) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Training : self.mData.mCalc &= ~ESM::Class::Training; }
        ));
        usertypeDefinition.set("offersSpellmaking", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Spellmaking) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Spellmaking : self.mData.mCalc &= ~ESM::Class::Spellmaking; }
        ));
        usertypeDefinition.set("offersEnchanting", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::Enchanting) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::Enchanting : self.mData.mCalc &= ~ESM::Class::Enchanting; }
        ));
        usertypeDefinition.set("offersRepairs", sol::property(
            [](ESM::Class& self) { return (self.mData.mCalc & ESM::Class::RepairItem) != 0; },
            [](ESM::Class& self, bool set) { set ? self.mData.mCalc |= ESM::Class::RepairItem : self.mData.mCalc &= ~ESM::Class::RepairItem; }
        ));

        usertypeDefinition.set("attributes", sol::readonly_property([](ESM::Class& self) { return std::ref(self.mData.mAttribute); }));
        usertypeDefinition.set("minorSkills", sol::readonly_property([](ESM::Class& self)
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            sol::table result = state.create_table();
            for (int i = 0; i < 5; i++)
            {
                result[i + 1] = self.mData.mSkills[i][0];
            }
            return result;
        }));
        usertypeDefinition.set("majorSkills", sol::readonly_property([](ESM::Class& self)
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            sol::table result = state.create_table();
            for (int i = 0; i < 5; i++)
            {
                result[i + 1] = self.mData.mSkills[i][1];
            }
            return result;
        }));

        // Finish up our usertype.
        state.set_usertype("tes3class", usertypeDefinition);
    }
}
