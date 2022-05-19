#include "types.hpp"

#include <components/esm3/loadbook.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Book> : std::false_type {};
}

namespace MWLua
{
    void addBookBindings(sol::table book, const Context& context)
    {   
        book["SKILL"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"Acrobatics", ESM::Skill::Acrobatics},
            {"Alchemy", ESM::Skill::Alchemy},
            {"Alteration", ESM::Skill::Alteration},
            {"Armorer", ESM::Skill::Armorer},
            {"Athletics", ESM::Skill::Athletics},
            {"Axe", ESM::Skill::Axe},
            {"Block", ESM::Skill::Block},
            {"BluntWeapon", ESM::Skill::BluntWeapon},
            {"Conjuration", ESM::Skill::Conjuration},
            {"Destruction", ESM::Skill::Destruction},
            {"Enchant", ESM::Skill::Enchant},
            {"HandToHand", ESM::Skill::HandToHand},
            {"HeavyArmor", ESM::Skill::HeavyArmor},
            {"Illusion", ESM::Skill::Illusion},
            {"LightArmor", ESM::Skill::LightArmor},
            {"LongBlade", ESM::Skill::LongBlade},
            {"Marksman", ESM::Skill::Marksman},
            {"MediumArmor", ESM::Skill::MediumArmor},
            {"Mercantile", ESM::Skill::Mercantile},
            {"Mysticism", ESM::Skill::Mysticism},
            {"Restoration", ESM::Skill::Restoration},
            {"Security", ESM::Skill::Security},
            {"ShortBlade", ESM::Skill::ShortBlade},
            {"Sneak", ESM::Skill::Sneak},
            {"Spear", ESM::Skill::Spear},
            {"Speechcraft", ESM::Skill::Speechcraft},
            {"Unarmored", ESM::Skill::Unarmored},
            }));

        const MWWorld::Store<ESM::Book>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Book>();
        book["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Book* { return obj.ptr().get<ESM::Book>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Book* { return store->find(recordId); });
        sol::usertype<ESM::Book> record = context.mLua->sol().new_usertype<ESM::Book>("ESM3_Book");
        record[sol::meta_function::to_string] = [](const ESM::Book& rec) { return "ESM3_Book[" + rec.mId + "]"; };
        record["id"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mModel; });
        record["mwscript"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mScript; });
        record["icon"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mIcon; });
        record["text"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mText; });
        record["enchant"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mEnchant; });
        record["isScroll"] = sol::readonly_property([](const ESM::Book& rec) -> bool { return rec.mData.mIsScroll; });
        record["skill"] = sol::readonly_property([](const ESM::Book& rec) -> int { return rec.mData.mSkillId; });
        record["value"] = sol::readonly_property([](const ESM::Book& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Book& rec) -> float { return rec.mData.mWeight; });
        record["enchantCapacity"] = sol::readonly_property([](const ESM::Book& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
