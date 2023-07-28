#include "types.hpp"

#include <components/esm3/loadweap.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Weapon> : std::false_type
    {
    };
}
#include <components/resource/resourcesystem.hpp>

namespace
{
    // Populates a weapon struct from a Lua table.
    ESM::Weapon tableToWeapon(const sol::table& rec)
    {
        ESM::Weapon weapon;
        weapon.mName = rec["name"];
        weapon.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        weapon.mIcon = rec["icon"];
        std::string_view enchantId = rec["enchant"].get<std::string_view>();
        weapon.mEnchant = ESM::RefId::deserializeText(enchantId);
        std::string_view scriptId = rec["mwscript"].get<std::string_view>();
        weapon.mScript = ESM::RefId::deserializeText(scriptId);
        weapon.mData.mFlags = 0;
        weapon.mRecordFlags = 0;
        if (rec["isMagical"])
            weapon.mData.mFlags |= ESM::Weapon::Magical;
        if (rec["isSilver"])
            weapon.mData.mFlags |= ESM::Weapon::Silver;
        int weaponType = rec["type"].get<int>();
        if (weaponType >= 0 && weaponType <= ESM::Weapon::MarksmanThrown)
            weapon.mData.mType = weaponType;
        else
            throw std::runtime_error("Invalid Weapon Type provided: " + std::to_string(weaponType));

        weapon.mData.mWeight = rec["weight"];
        weapon.mData.mValue = rec["value"];
        weapon.mData.mHealth = rec["health"];
        weapon.mData.mSpeed = rec["speed"];
        weapon.mData.mReach = rec["reach"];
        weapon.mData.mEnchant = std::round(rec["enchantCapacity"].get<float>() * 10);
        weapon.mData.mChop[0] = rec["chopMinDamage"];
        weapon.mData.mChop[1] = rec["chopMaxDamage"];
        weapon.mData.mSlash[0] = rec["slashMinDamage"];
        weapon.mData.mSlash[1] = rec["slashMaxDamage"];
        weapon.mData.mThrust[0] = rec["thrustMinDamage"];
        weapon.mData.mThrust[1] = rec["thrustMaxDamage"];

        return weapon;
    }
}

namespace MWLua
{
    void addWeaponBindings(sol::table weapon, const Context& context)
    {
        weapon["TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
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

        sol::usertype<ESM::Weapon> record = context.mLua->sol().new_usertype<ESM::Weapon>("ESM3_Weapon");
        record[sol::meta_function::to_string]
            = [](const ESM::Weapon& rec) -> std::string { return "ESM3_Weapon[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Weapon& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Weapon& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"] = sol::readonly_property(
            [](const ESM::Weapon& rec) -> std::string { return rec.mEnchant.serializeText(); });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mScript.serializeText(); });
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
