#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadarmo.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

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
        if (rec["template"] != sol::nil)
            armor = LuaUtil::cast<ESM::Armor>(rec["template"]);
        else
            armor.blank();
        if (rec["name"] != sol::nil)
            armor.mName = rec["name"];
        if (rec["model"] != sol::nil)
            armor.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            armor.mIcon = rec["icon"];
        if (rec["enchant"] != sol::nil)
        {
            std::string_view enchantId = rec["enchant"].get<std::string_view>();
            armor.mEnchant = ESM::RefId::deserializeText(enchantId);
        }
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            armor.mScript = ESM::RefId::deserializeText(scriptId);
        }

        if (rec["weight"] != sol::nil)
            armor.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            armor.mData.mValue = rec["value"];
        if (rec["type"] != sol::nil)
        {
            int armorType = rec["type"].get<int>();
            if (armorType >= 0 && armorType <= ESM::Armor::RBracer)
                armor.mData.mType = armorType;
            else
                throw std::runtime_error("Invalid Armor Type provided: " + std::to_string(armorType));
        }
        if (rec["health"] != sol::nil)
            armor.mData.mHealth = rec["health"];
        if (rec["baseArmor"] != sol::nil)
            armor.mData.mArmor = rec["baseArmor"];
        if (rec["enchantCapacity"] != sol::nil)
            armor.mData.mEnchant = static_cast<int32_t>(std::round(rec["enchantCapacity"].get<float>() * 10));

        return armor;
    }
}

namespace MWLua
{
    void addArmorBindings(sol::table armor, const Context& context)
    {
        sol::state_view lua = context.sol();
        armor["TYPE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            {
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
        sol::usertype<ESM::Armor> record = lua.new_usertype<ESM::Armor>("ESM3_Armor");
        record[sol::meta_function::to_string]
            = [](const ESM::Armor& rec) -> std::string { return "ESM3_Armor[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["icon"] = sol::readonly_property([vfs](const ESM::Armor& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(rec.mIcon), *vfs);
        });
        record["enchant"] = sol::readonly_property([](const ESM::Armor& rec) -> ESM::RefId { return rec.mEnchant; });
        record["mwscript"] = sol::readonly_property([](const ESM::Armor& rec) -> ESM::RefId { return rec.mScript; });
        record["weight"] = sol::readonly_property([](const ESM::Armor& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mType; });
        record["health"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mHealth; });
        record["baseArmor"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mArmor; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Armor& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
