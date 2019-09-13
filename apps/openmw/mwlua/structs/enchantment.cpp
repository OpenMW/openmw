#include "enchantment.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadench.hpp>

namespace MWLua
{
    void bindTES3Enchantment()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Enchantment>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Enchantment::mId);

        usertypeDefinition.set("castType", sol::property(
            [](ESM::Enchantment& self) { return self.mData.mType; },
            [](ESM::Enchantment& self, int value) { self.mData.mType = value; }
        ));
        usertypeDefinition.set("chargeCost", sol::property(
            [](ESM::Enchantment& self) { return self.mData.mCost; },
            [](ESM::Enchantment& self, int value) { self.mData.mCost = value; }
        ));
        usertypeDefinition.set("maxCharge", sol::property(
            [](ESM::Enchantment& self) { return self.mData.mCharge; },
            [](ESM::Enchantment& self, int value) { self.mData.mCharge = value; }
        ));
        usertypeDefinition.set("autoCalc", sol::property(
            [](ESM::Enchantment& self) { return self.mData.mAutocalc > 0; },
            [](ESM::Enchantment& self, bool value) { self.mData.mAutocalc = value; }
        ));

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("effects", sol::readonly_property([](ESM::Enchantment& self) { return &self.mEffects.mList; }));

        usertypeDefinition.set("getFirstIndexOfEffect", [](ESM::Enchantment& self, int effectId)
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
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Enchantment::mId);

        // Finish up our usertype.
        state.set_usertype("tes3enchantment", usertypeDefinition);
    }
}
