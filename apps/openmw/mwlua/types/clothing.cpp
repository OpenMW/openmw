#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadclot.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

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
        if (rec["template"] != sol::nil)
            clothing = LuaUtil::cast<ESM::Clothing>(rec["template"]);
        else
            clothing.blank();

        if (rec["name"] != sol::nil)
            clothing.mName = rec["name"];
        if (rec["model"] != sol::nil)
            clothing.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            clothing.mIcon = rec["icon"];
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            clothing.mScript = ESM::RefId::deserializeText(scriptId);
        }

        if (rec["enchant"] != sol::nil)
        {
            std::string_view enchantId = rec["enchant"].get<std::string_view>();
            clothing.mEnchant = ESM::RefId::deserializeText(enchantId);
        }
        if (rec["enchantCapacity"] != sol::nil)
            clothing.mData.mEnchant = static_cast<int16_t>(std::round(rec["enchantCapacity"].get<float>() * 10));
        if (rec["weight"] != sol::nil)
            clothing.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            clothing.mData.mValue = rec["value"];
        if (rec["type"] != sol::nil)
        {
            int clothingType = rec["type"].get<int>();
            if (clothingType >= 0 && clothingType <= ESM::Clothing::Amulet)
                clothing.mData.mType = clothingType;
            else
                throw std::runtime_error("Invalid Clothing Type provided: " + std::to_string(clothingType));
        }
        return clothing;
    }
}
namespace MWLua
{
    void addClothingBindings(sol::table clothing, const Context& context)
    {
        clothing["createRecordDraft"] = tableToClothing;

        sol::state_view lua = context.sol();
        clothing["TYPE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            {
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

        sol::usertype<ESM::Clothing> record = lua.new_usertype<ESM::Clothing>("ESM3_Clothing");
        record[sol::meta_function::to_string]
            = [](const ESM::Clothing& rec) -> std::string { return "ESM3_Clothing[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Clothing& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Clothing& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["icon"] = sol::readonly_property([vfs](const ESM::Clothing& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"] = sol::readonly_property([](const ESM::Clothing& rec) -> ESM::RefId { return rec.mEnchant; });
        record["mwscript"] = sol::readonly_property([](const ESM::Clothing& rec) -> ESM::RefId { return rec.mScript; });
        record["weight"] = sol::readonly_property([](const ESM::Clothing& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Clothing& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Clothing& rec) -> int { return rec.mData.mType; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Clothing& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
