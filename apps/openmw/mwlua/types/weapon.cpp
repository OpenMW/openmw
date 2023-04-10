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
