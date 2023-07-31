#include "types.hpp"

#include <components/esm3/loadarmo.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Armor> : std::false_type
    {
    };
}
namespace
{
    // Populates an armor struct from a Lua table.
    ESM::Armor tableToArmor(const sol::table& rec)
    {
        ESM::Armor armor;
        armor.mName = rec["name"];
        armor.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        armor.mIcon = rec["icon"];
        std::string_view enchantId = rec["enchant"].get<std::string_view>();
        armor.mEnchant = ESM::RefId::deserializeText(enchantId);
        std::string_view scriptId = rec["mwscript"].get<std::string_view>();
        armor.mScript = ESM::RefId::deserializeText(scriptId);

        armor.mData.mWeight = rec["weight"];
        armor.mData.mValue = rec["value"];
        int armorType = rec["type"].get<int>();
        if (armorType >= 0 && armorType <= ESM::Armor::RBracer)
            armor.mData.mType = armorType;
        else
            throw std::runtime_error("Invalid Armor Type provided: " + std::to_string(armorType));
        armor.mData.mHealth = rec["health"];
        armor.mData.mArmor = rec["baseArmor"];
        armor.mData.mEnchant = std::round(rec["enchantCapacity"].get<float>() * 10);
        armor.mRecordFlags = 0;

        return armor;
    }
}

namespace MWLua
{
    void addArmorBindings(sol::table armor, const Context& context)
    {
        armor["TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            { "Helmet", ESM::Armor::Helmet },
            { "Cuirass", ESM::Armor::Cuirass },
            { "LPauldron", ESM::Armor::LPauldron },
            { "RPauldron", ESM::Armor::RPauldron },
            { "Greaves", ESM::Armor::Greaves },
            { "Boots", ESM::Armor::Boots },
            { "LGauntlet", ESM::Armor::LGauntlet },
            { "RGauntlet", ESM::Armor::RGauntlet },
            { "Shield", ESM::Armor::Shield },
            { "LBracer", ESM::Armor::LBracer },
            { "RBracer", ESM::Armor::RBracer },
        }));

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Armor>(armor, context);

        armor["createRecordDraft"] = tableToArmor;
        sol::usertype<ESM::Armor> record = context.mLua->sol().new_usertype<ESM::Armor>("ESM3_Armor");
        record[sol::meta_function::to_string]
            = [](const ESM::Armor& rec) -> std::string { return "ESM3_Armor[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Armor& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Armor& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mEnchant.serializeText(); });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Armor& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mType; });
        record["health"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mHealth; });
        record["baseArmor"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mArmor; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Armor& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
