#include "alchemy.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/esmstore.hpp"

namespace MWLua
{
    ESM::Potion* createPotion(sol::table params)
    {
        // Do we already have an object of this ID?
        std::string id = getOptionalParam<std::string>(params, "id", "");
        MWWorld::Store<ESM::Potion> *alchemyStore = const_cast<MWWorld::Store<ESM::Potion>*>(&MWBase::Environment::get().getWorld()->getStore().get<ESM::Potion>());
        const ESM::Potion *existingPotion = alchemyStore->search(id);
        if (id.empty() || existingPotion != nullptr)
        {
            return nullptr;
        }

        // Create new alchemy object.
        ESM::Potion alchemy;
        alchemy.mId = id;

        // Set name. Defaults to "Potion"
        std::string name = getOptionalParam<std::string>(params, "name", "Potion");
        alchemy.mName = name;

        // Set script.
        std::string script = getOptionalParam<std::string>(params, "script", "");
        alchemy.mScript = script;

        // Set model.
        std::string model = getOptionalParam<std::string>(params, "mesh", "m\\Misc_Potion_Bargain_01.nif");
        alchemy.mModel = model;

        // Set texture.
        std::string icon = getOptionalParam<std::string>(params, "icon", "m\\Tx_potion_bargain_01.tga");
        alchemy.mIcon = icon;

        // Get other simple values.
        alchemy.mData.mWeight = getOptionalParam<double>(params, "weight", 0.0);
        alchemy.mData.mValue = getOptionalParam<double>(params, "value", 0.0);
        alchemy.mData.mAutoCalc = getOptionalParam<double>(params, "autoCalc", true);

        // Assign effects.
        sol::optional<sol::table> effects = params["effects"];
        if (effects)
        {
            for (int i = 1; i <= 8; i++)
            {
                sol::optional<sol::table> effectParams = effects.value()[i];
                if (!effectParams)
                {
                    break;
                }

                ESM::ENAMstruct effect;

                effect.mEffectID = getOptionalParam<short>(effectParams, "id", -1);
                effect.mSkill = getOptionalParam<signed char>(effectParams, "skill", -1);
                effect.mAttribute = getOptionalParam<signed char>(effectParams, "attribute", -1);
                effect.mRange = getOptionalParam<int>(effectParams, "range", 0);
                effect.mArea = getOptionalParam<int>(effectParams, "radius", 0);
                effect.mDuration = getOptionalParam<int>(effectParams, "duration", 0);
                effect.mMagnMin = getOptionalParam<int>(effectParams, "min", 0);
                effect.mMagnMax = getOptionalParam<int>(effectParams, "max", 0);

                alchemy.mEffects.mList.push_back(effect);
            }
        }

        alchemyStore->insert(alchemy);

        return const_cast<ESM::Potion*>(alchemyStore->find(id));
    }

    void bindTES3Alchemy()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Potion>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Potion::mId);
        usertypeDefinition.set("mesh", &ESM::Potion::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Potion& self) { return self.mIcon; },
            [](ESM::Potion& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Potion::mScript);
        usertypeDefinition.set("name", &ESM::Potion::mName);
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Potion& self) { return self.mData.mWeight; },
            [](ESM::Potion& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Potion& self) { return self.mData.mValue; },
            [](ESM::Potion& self, int value) { self.mData.mValue = value; }
        ));
         usertypeDefinition.set("autoCalc", sol::property(
            [](ESM::Potion& self) { return self.mData.mAutoCalc > 0; },
            [](ESM::Potion& self, bool value) { self.mData.mAutoCalc = value; }
        ));

        // Basic function binding.
        usertypeDefinition.set("create", &createPotion);

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("effects", sol::readonly_property([](ESM::Potion& self) { return &self.mEffects.mList; }));

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Potion::mId);

        // Finish up our usertype.
        state.set_usertype("tes3alchemy", usertypeDefinition);
    }
}
