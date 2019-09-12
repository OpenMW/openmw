#include "spell.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwmechanics/spellcasting.hpp"

#include "../../mwworld/class.hpp"
#include "../../mwworld/esmstore.hpp"

namespace MWLua
{
    ESM::Spell* createSpell(std::string id, std::string name)
    {
        // Make sure a spell doesn't already exist with this id.
        MWWorld::Store<ESM::Spell> *spells = const_cast<MWWorld::Store<ESM::Spell>*>(&MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>());
        const ESM::Spell *existingSpell = spells->search(id);
        if (existingSpell != nullptr)
        {
            return nullptr;
        }

        // Limit name to 31 characters.
        if (name.length() > 31)
        {
            return nullptr;
        }

        ESM::Spell newSpell;
        newSpell.blank();
        newSpell.mData.mCost = 1;

        // Set id/name.
        newSpell.mId = id;
        newSpell.mName = name;

        // Add object to the game.
        spells->insert(newSpell);

        // Finally return the newSpell.
        return const_cast<ESM::Spell*>(spells->find(id));
    }

    void bindTES3Spell()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Spell>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Spell::mId);

        usertypeDefinition.set("name", &ESM::Spell::mName);

        usertypeDefinition.set("castType", sol::property(
            [](ESM::Spell& self) { return self.mData.mType; },
            [](ESM::Spell& self, int value) { self.mData.mType = value; }
        ));

        usertypeDefinition.set("magickaCost", sol::property(
            [](ESM::Spell& self) { return self.mData.mCost; },
            [](ESM::Spell& self, int value) { self.mData.mCost = value; }
        ));

        // Allow access to flags.
        usertypeDefinition.set("flags", sol::property(
            [](ESM::Spell& self) { return self.mData.mFlags; },
            [](ESM::Spell& self, int value) { self.mData.mFlags = value; }
        ));

        // User-friendly access to those flags.
        usertypeDefinition.set("autoCalc", sol::property(
            [](ESM::Spell& self) { return (self.mData.mFlags & ESM::Spell::F_Autocalc) != 0; },
            [](ESM::Spell& self, bool set) { set ? self.mData.mFlags |= ESM::Spell::F_Autocalc : self.mData.mFlags &= ~ESM::Spell::F_Autocalc; }
        ));
        usertypeDefinition.set("startSpell", sol::property(
            [](ESM::Spell& self) { return (self.mData.mFlags & ESM::Spell::F_PCStart) != 0; },
            [](ESM::Spell& self, bool set) { set ? self.mData.mFlags |= ESM::Spell::F_PCStart : self.mData.mFlags &= ~ESM::Spell::F_PCStart; }
        ));
        usertypeDefinition.set("alwaysSucceeds", sol::property(
            [](ESM::Spell& self) { return (self.mData.mFlags & ESM::Spell::F_Always) != 0; },
            [](ESM::Spell& self, bool set) { set ? self.mData.mFlags |= ESM::Spell::F_Always : self.mData.mFlags &= ~ESM::Spell::F_Always; }
        ));

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("effects", sol::readonly_property([](ESM::Spell& self) { return &self.mEffects.mList; }));

        // Basic function binding.
        usertypeDefinition.set("create", &createSpell);

        usertypeDefinition.set("calculateCastChance",
            [](ESM::Spell& self, sol::table params) -> float
            {
                bool checkMagicka = getOptionalParam<bool>(params, "checkMagicka", true);
                bool cap = getOptionalParam<bool>(params, "cap", true);
                MWWorld::Ptr actor = getOptionalParamReference(params, "caster");
                if (actor.isEmpty() || !actor.getClass().isActor())
                    return 0.f;

                return MWMechanics::getSpellSuccessChance(&self, actor, nullptr, cap, checkMagicka);
            }
        );

        usertypeDefinition.set("getFirstIndexOfEffect", [](ESM::Spell& self, int effectId)
        {
            for (size_t i = 0; i < self.mEffects.mList.size(); i++)
            {
                if (self.mEffects.mList[i].mEffectID == effectId)
                {
                    return int(i);
                }
            }
            return -1;
        });

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Spell::mId);

        // Finish up our usertype.
        state.set_usertype("tes3spell", usertypeDefinition);
    }
}
