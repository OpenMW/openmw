#include "types.hpp"

#include <components/esm3/loadweap.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Weapon> : std::false_type {};
}

namespace MWLua
{
    void addWeaponBindings(sol::table weapon, const Context& context)
    {
        weapon["TYPE"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"ShortBladeOneHand", ESM::Weapon::ShortBladeOneHand},
            {"LongBladeOneHand", ESM::Weapon::LongBladeOneHand},
            {"LongBladeTwoHand", ESM::Weapon::LongBladeTwoHand},
            {"BluntOneHand", ESM::Weapon::BluntOneHand},
            {"BluntTwoClose", ESM::Weapon::BluntTwoClose},
            {"BluntTwoWide", ESM::Weapon::BluntTwoWide},
            {"SpearTwoWide", ESM::Weapon::SpearTwoWide},
            {"AxeOneHand", ESM::Weapon::AxeOneHand},
            {"AxeTwoHand", ESM::Weapon::AxeTwoHand},
            {"MarksmanBow", ESM::Weapon::MarksmanBow},
            {"MarksmanCrossbow", ESM::Weapon::MarksmanCrossbow},
            {"MarksmanThrown", ESM::Weapon::MarksmanThrown},
            {"Arrow", ESM::Weapon::Arrow},
            {"Bolt", ESM::Weapon::Bolt},
        }));

        const MWWorld::Store<ESM::Weapon>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Weapon>();
        weapon["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Weapon* { return obj.ptr().get<ESM::Weapon>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Weapon* { return store->find(recordId); });
        sol::usertype<ESM::Weapon> record = context.mLua->sol().new_usertype<ESM::Weapon>("ESM3_Weapon");
        record[sol::meta_function::to_string] = [](const ESM::Weapon& rec) -> std::string { return "ESM3_Weapon[" + rec.mId + "]"; };
        record["id"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mModel; });
        record["icon"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mIcon; });
        record["enchant"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mEnchant; });
        record["mwscript"] = sol::readonly_property([](const ESM::Weapon& rec) -> std::string { return rec.mScript; });
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
        record["enchant"] = sol::readonly_property([](const ESM::Weapon& rec) -> float { return rec.mData.mEnchant * 0.1f; });
        record["chopMinDamage"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mChop[0]; });
        record["chopMaxDamage"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mChop[1]; });
        record["slashMinDamage"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mSlash[0]; });
        record["slashMaxDamage"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mSlash[1]; });
        record["thrustMinDamage"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mThrust[0]; });
        record["thrustMaxDamage"] = sol::readonly_property([](const ESM::Weapon& rec) -> int { return rec.mData.mThrust[1]; });
    }

}
