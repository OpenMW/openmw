#include "ingredient.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadingr.hpp>

namespace MWLua
{
    void bindTES3Ingredient()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Ingredient>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Ingredient::mId);
        usertypeDefinition.set("mesh", &ESM::Ingredient::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Ingredient& self) { return self.mIcon; },
            [](ESM::Ingredient& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Ingredient::mScript);
        usertypeDefinition.set("name", &ESM::Ingredient::mName);
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Ingredient& self) { return self.mData.mWeight; },
            [](ESM::Ingredient& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Ingredient& self) { return self.mData.mValue; },
            [](ESM::Ingredient& self, int value) { self.mData.mValue = value; }
        ));

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("effects", sol::readonly_property([](ESM::Ingredient& self) { return std::ref(self.mData.mEffectID); }));
        usertypeDefinition.set("effectSkillIds", sol::readonly_property([](ESM::Ingredient& self) { return std::ref(self.mData.mSkills); }));
        usertypeDefinition.set("effectAttributeIds", sol::readonly_property([](ESM::Ingredient& self) { return std::ref(self.mData.mAttributes); }));

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Ingredient::mId);

        // Finish up our usertype.
        state.set_usertype("tes3ingredient", usertypeDefinition);
    }
}
