#include "types.hpp"

#include <components/esm3/loadbook.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>
#include <components/esm3/loadskil.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Book> : std::false_type
    {
    };
}

namespace MWLua
{
    void addBookBindings(sol::table book, const Context& context)
    {
        sol::table skill(context.mLua->sol(), sol::create);
        book["SKILL"] = LuaUtil::makeStrictReadOnly(skill);
        for (int id = ESM::Skill::Block; id < ESM::Skill::Length; ++id)
        {
            std::string skillName = Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[id]);
            skill[skillName] = skillName;
        }

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Book>(book);

        sol::usertype<ESM::Book> record = context.mLua->sol().new_usertype<ESM::Book>("ESM3_Book");
        record[sol::meta_function::to_string]
            = [](const ESM::Book& rec) { return "ESM3_Book[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Book& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mScript.serializeText(); });
        record["icon"] = sol::readonly_property([vfs](const ESM::Book& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["text"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mText; });
        record["enchant"]
            = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mEnchant.serializeText(); });
        record["isScroll"] = sol::readonly_property([](const ESM::Book& rec) -> bool { return rec.mData.mIsScroll; });
        record["value"] = sol::readonly_property([](const ESM::Book& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Book& rec) -> float { return rec.mData.mWeight; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Book& rec) -> float { return rec.mData.mEnchant * 0.1f; });
        record["skill"] = sol::readonly_property([](const ESM::Book& rec) -> sol::optional<std::string> {
            if (rec.mData.mSkillId >= 0)
                return Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[rec.mData.mSkillId]);
            else
                return sol::nullopt;
        });
    }
}
