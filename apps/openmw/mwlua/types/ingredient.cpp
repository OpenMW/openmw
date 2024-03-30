#include "types.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Ingredient> : std::false_type
    {
    };
}

namespace MWLua
{
    void addIngredientBindings(sol::table ingredient, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Ingredient>(ingredient, context);

        sol::usertype<ESM::Ingredient> record = context.mLua->sol().new_usertype<ESM::Ingredient>(("ESM3_Ingredient"));
        record[sol::meta_function::to_string]
            = [](const ESM::Ingredient& rec) { return "ESM3_Ingredient[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Ingredient& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Ingredient& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Ingredient& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Ingredient& rec) -> std::string { return rec.mScript.serializeText(); });
        record["icon"] = sol::readonly_property([vfs](const ESM::Ingredient& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["weight"]
            = sol::readonly_property([](const ESM::Ingredient& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Ingredient& rec) -> int { return rec.mData.mValue; });
        record["effects"] = sol::readonly_property([context](const ESM::Ingredient& rec) -> sol::table {
            sol::table res(context.mLua->sol(), sol::create);
            for (size_t i = 0; i < 4; ++i)
            {
                if (rec.mData.mEffectID[i] < 0)
                    continue;
                ESM::IndexedENAMstruct effect;
                effect.mData.mEffectID = rec.mData.mEffectID[i];
                effect.mData.mSkill = rec.mData.mSkills[i];
                effect.mData.mAttribute = rec.mData.mAttributes[i];
                effect.mData.mRange = ESM::RT_Self;
                effect.mData.mArea = 0;
                effect.mData.mDuration = 0;
                effect.mData.mMagnMin = 0;
                effect.mData.mMagnMax = 0;
                effect.mIndex = i;
                res[i + 1] = effect;
            }
            return res;
        });
    }
}
