#include "types.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

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

namespace
{
    // Populates a book struct from a Lua table.
    ESM::Book tableToBook(const sol::table& rec)
    {
        ESM::Book book;
        book.mName = rec["name"];
        book.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        book.mIcon = rec["icon"];
        book.mText = rec["text"];
        std::string_view enchantId = rec["enchant"].get<std::string_view>();
        book.mEnchant = ESM::RefId::deserializeText(enchantId);

        book.mData.mEnchant = std::round(rec["enchantCapacity"].get<float>() * 10);
        std::string_view scriptId = rec["mwscript"].get<std::string_view>();
        book.mScript = ESM::RefId::deserializeText(scriptId);
        book.mData.mWeight = rec["weight"];
        book.mData.mValue = rec["value"];
        book.mData.mIsScroll = rec["isScroll"];

        std::string_view skill = rec["skill"].get<std::string_view>();

        book.mData.mSkillId = -1;
        if (skill.length() > 0)
        {
            for (std::size_t i = 0; i < std::size(ESM::Skill::sSkillNames); ++i)
            {
                if (Misc::StringUtils::ciEqual(ESM::Skill::sSkillNames[i], skill))
                    book.mData.mSkillId = i;
            }
            if (book.mData.mSkillId == -1)
                throw std::runtime_error("Incorrect skill: " + std::string(skill));
        }
        return book;
    }
}

namespace MWLua
{
    void addBookBindings(sol::table book, const Context& context)
    {
        // types.book.SKILL is deprecated (core.SKILL should be used instead)
        // TODO: Remove book.SKILL after branching 0.49
        sol::table skill(context.mLua->sol(), sol::create);
        book["SKILL"] = LuaUtil::makeStrictReadOnly(skill);
        book["createRecordDraft"] = tableToBook;
        for (int id = 0; id < ESM::Skill::Length; ++id)
        {
            std::string skillName = Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[id]);
            skill[skillName] = skillName;
        }

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Book>(book, context);

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
