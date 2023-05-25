#include "types.hpp"

#include <components/esm3/loadclot.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Clothing> : std::false_type
    {
    };
}
namespace
{
    // Populates a clothing struct from a Lua table.
    ESM::Clothing tableToClothing(const sol::table& rec)
    {
        ESM::Clothing clothing;
        clothing.mName = rec["name"];
        clothing.mModel = rec["model"];
        clothing.mIcon = rec["icon"];
        std::string_view scriptId = rec["mwscript"].get<std::string_view>();
        clothing.mScript = ESM::RefId::deserializeText(scriptId);
        clothing.mData.mEnchant = std::round(rec["enchantCapacity"].get<float>() * 10);
        std::string_view enchantId = rec["enchant"].get<std::string_view>();
        clothing.mEnchant = ESM::RefId::deserializeText(enchantId);
        clothing.mData.mWeight = rec["weight"];
        clothing.mData.mValue = rec["value"];
        int clothingType = rec["type"].get<int>();
        if (clothingType >= 0 && clothingType <= ESM::Clothing::Amulet)
            clothing.mData.mType = clothingType;
        else
            throw std::runtime_error("Invalid Clothing Type provided: " + std::to_string(clothingType));
        return clothing;
    }
}
namespace MWLua
{
    void addClothingBindings(sol::table clothing, const Context& context)
    {
        clothing["createRecordDraft"] = tableToClothing;

        clothing["TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            { "Amulet", ESM::Clothing::Amulet },
            { "Belt", ESM::Clothing::Belt },
            { "LGlove", ESM::Clothing::LGlove },
            { "Pants", ESM::Clothing::Pants },
            { "RGlove", ESM::Clothing::RGlove },
            { "Ring", ESM::Clothing::Ring },
            { "Robe", ESM::Clothing::Robe },
            { "Shirt", ESM::Clothing::Shirt },
            { "Shoes", ESM::Clothing::Shoes },
            { "Skirt", ESM::Clothing::Skirt },
        }));

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Clothing>(clothing, context);

        sol::usertype<ESM::Clothing> record = context.mLua->sol().new_usertype<ESM::Clothing>("ESM3_Clothing");
        record[sol::meta_function::to_string]
            = [](const ESM::Clothing& rec) -> std::string { return "ESM3_Clothing[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Clothing& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Clothing& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Clothing& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Clothing& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"] = sol::readonly_property(
            [](const ESM::Clothing& rec) -> std::string { return rec.mEnchant.serializeText(); });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Clothing& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Clothing& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Clothing& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Clothing& rec) -> int { return rec.mData.mType; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Clothing& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
