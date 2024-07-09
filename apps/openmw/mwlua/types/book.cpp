#include "types.hpp"

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

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
        if (rec["template"] != sol::nil)
            book = LuaUtil::cast<ESM::Book>(rec["template"]);
        else
        {
            book.blank();
            book.mData.mSkillId = -1;
        }
        if (rec["name"] != sol::nil)
            book.mName = rec["name"];
        if (rec["model"] != sol::nil)
            book.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            book.mIcon = rec["icon"];
        if (rec["text"] != sol::nil)
            book.mText = rec["text"];
        if (rec["enchant"] != sol::nil)
        {
            std::string_view enchantId = rec["enchant"].get<std::string_view>();
            book.mEnchant = ESM::RefId::deserializeText(enchantId);
        }

        if (rec["enchantCapacity"] != sol::nil)
            book.mData.mEnchant = std::round(rec["enchantCapacity"].get<float>() * 10);
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            book.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["weight"] != sol::nil)
            book.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            book.mData.mValue = rec["value"];
        if (rec["isScroll"] != sol::nil)
            book.mData.mIsScroll = rec["isScroll"] ? 1 : 0;

        if (rec["skill"] != sol::nil)
        {
            ESM::RefId skill = ESM::RefId::deserializeText(rec["skill"].get<std::string_view>());

            book.mData.mSkillId = -1;
            if (!skill.empty())
            {
                book.mData.mSkillId = ESM::Skill::refIdToIndex(skill);
                if (book.mData.mSkillId == -1)
                    throw std::runtime_error("Incorrect skill: " + skill.toDebugString());
            }
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
            std::string skillName = ESM::Skill::indexToRefId(id).serializeText();
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
        record["model"] = sol::readonly_property(
            [](const ESM::Book& rec) -> std::string { return Misc::ResourceHelpers::correctMeshPath(rec.mModel); });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Book& rec) -> sol::optional<std::string> { return LuaUtil::serializeRefId(rec.mScript); });
        record["icon"] = sol::readonly_property([vfs](const ESM::Book& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["text"] = sol::readonly_property([](const ESM::Book& rec) -> std::string { return rec.mText; });
        record["enchant"] = sol::readonly_property(
            [](const ESM::Book& rec) -> sol::optional<std::string> { return LuaUtil::serializeRefId(rec.mEnchant); });
        record["isScroll"] = sol::readonly_property([](const ESM::Book& rec) -> bool { return rec.mData.mIsScroll; });
        record["value"] = sol::readonly_property([](const ESM::Book& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Book& rec) -> float { return rec.mData.mWeight; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Book& rec) -> float { return rec.mData.mEnchant * 0.1f; });
        record["skill"] = sol::readonly_property([](const ESM::Book& rec) -> sol::optional<std::string> {
            ESM::RefId skill = ESM::Skill::indexToRefId(rec.mData.mSkillId);
            return LuaUtil::serializeRefId(skill);
        });
    }
}
