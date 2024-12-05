#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadweap.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Weapon> : std::false_type
    {
    };
}

namespace
{
    // Populates a weapon struct from a Lua table.
    ESM::Weapon tableToWeapon(const sol::table& rec)
    {
        ESM::Weapon weapon;
        if (rec["template"] != sol::nil)
            weapon = LuaUtil::cast<ESM::Weapon>(rec["template"]);
        else
            weapon.blank();

        if (rec["name"] != sol::nil)
            weapon.mName = rec["name"];
        if (rec["model"] != sol::nil)
            weapon.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            weapon.mIcon = rec["icon"];
        if (rec["enchant"] != sol::nil)
        {
            std::string_view enchantId = rec["enchant"].get<std::string_view>();
            weapon.mEnchant = ESM::RefId::deserializeText(enchantId);
        }
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            weapon.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (auto isMagical = rec["isMagical"]; isMagical != sol::nil)
        {
            if (isMagical)
                weapon.mData.mFlags |= ESM::Weapon::Magical;
            else
                weapon.mData.mFlags &= ~ESM::Weapon::Magical;
        }
        if (auto isSilver = rec["isSilver"]; isSilver != sol::nil)
        {
            if (isSilver)
                weapon.mData.mFlags |= ESM::Weapon::Silver;
            else
                weapon.mData.mFlags &= ~ESM::Weapon::Silver;
        }

        if (rec["type"] != sol::nil)
        {
            int weaponType = rec["type"].get<int>();
            if (weaponType >= 0 && weaponType <= ESM::Weapon::MarksmanThrown)
                weapon.mData.mType = weaponType;
            else
                throw std::runtime_error("Invalid Weapon Type provided: " + std::to_string(weaponType));
        }
        if (rec["weight"] != sol::nil)
            weapon.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            weapon.mData.mValue = rec["value"];
        if (rec["health"] != sol::nil)
            weapon.mData.mHealth = rec["health"];
        if (rec["speed"] != sol::nil)
            weapon.mData.mSpeed = rec["speed"];
        if (rec["reach"] != sol::nil)
            weapon.mData.mReach = rec["reach"];
        if (rec["enchantCapacity"] != sol::nil)
            weapon.mData.mEnchant = std::round(rec["enchantCapacity"].get<float>() * 10);
        if (rec["chopMinDamage"] != sol::nil)
            weapon.mData.mChop[0] = rec["chopMinDamage"];
        if (rec["chopMaxDamage"] != sol::nil)
            weapon.mData.mChop[1] = rec["chopMaxDamage"];
        if (rec["slashMinDamage"] != sol::nil)
            weapon.mData.mSlash[0] = rec["slashMinDamage"];
        if (rec["slashMaxDamage"] != sol::nil)
            weapon.mData.mSlash[1] = rec["slashMaxDamage"];
        if (rec["thrustMinDamage"] != sol::nil)
            weapon.mData.mThrust[0] = rec["thrustMinDamage"];
        if (rec["thrustMaxDamage"] != sol::nil)
            weapon.mData.mThrust[1] = rec["thrustMaxDamage"];

        return weapon;
    }
}

namespace MWLua
{
    void addWeaponBindings(sol::table weapon, const Context& context)
    {
        sol::state_view lua = context.sol();
        weapon["TYPE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            {
                { "ShortBladeOneHand", ESM::Weapon::ShortBladeOneHand },
                { "LongBladeOneHand", ESM::Weapon::LongBladeOneHand },
                { "LongBladeTwoHand", ESM::Weapon::LongBladeTwoHand },
                { "BluntOneHand", ESM::Weapon::BluntOneHand },
                { "BluntTwoClose", ESM::Weapon::BluntTwoClose },
                { "BluntTwoWide", ESM::Weapon::BluntTwoWide },
                { "SpearTwoWide", ESM::Weapon::SpearTwoWide },
                { "AxeOneHand", ESM::Weapon::AxeOneHand },
                { "AxeTwoHand", ESM::Weapon::AxeTwoHand },
                { "MarksmanBow", ESM::Weapon::MarksmanBow },
                { "MarksmanCrossbow", ESM::Weapon::MarksmanCrossbow },
                { "MarksmanThrown", ESM::Weapon::MarksmanThrown },
                { "Arrow", ESM::Weapon::Arrow },
                { "Bolt", ESM::Weapon::Bolt },
            }));

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Weapon>(weapon, context);
        weapon["createRecordDraft"] = tableToWeapon;

        sol::usertype<ESM::Weapon> record = lua.new_usertype<ESM::Weapon>("ESM3_Weapon");
        record[sol::meta_function::to_string]
            = [](const ESM::Weapon& rec) -> std::string { return "ESM3_Weapon[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["icon"] = sol::readonly_property([vfs](const ESM::Weapon& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"] = sol::readonly_property(
            [](const ESM::Weapon& rec) -> sol::optional<std::string> { return LuaUtil::serializeRefId(rec.mEnchant); });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Weapon& rec) -> sol::optional<std::string> { return LuaUtil::serializeRefId(rec.mScript); });
        record["isMagical"] = sol::readonly_property(
            [](const ESM::Weapon& rec) -> bool { return rec.mData.mFlags & ESM::Weapon::Magical; });
        record["isSilver"] = sol::readonly_property(
            [](const ESM::Weapon& rec) -> bool { return rec.mData.mFlags & ESM::Weapon::Silver; });
        record["weight"] = sol::readonly_property([](const ESM::Weapon& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mType; });
        record["health"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mHealth; });
        record["speed"] = sol::readonly_property([](const ESM::Weapon& rec) -> float { return rec.mData.mSpeed; });
        record["reach"] = sol::readonly_property([](const ESM::Weapon& rec) -> float { return rec.mData.mReach; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> float { return rec.mData.mEnchant * 0.1f; });
        record["chopMinDamage"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mChop[0]; });
        record["chopMaxDamage"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mChop[1]; });
        record["slashMinDamage"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mSlash[0]; });
        record["slashMaxDamage"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mSlash[1]; });
        record["thrustMinDamage"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mThrust[0]; });
        record["thrustMaxDamage"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mThrust[1]; });
    }

}
