#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/finitevalues.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Book> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Book[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Book::mName);
            Types::addModelProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Book::mScript);
            Types::addIconProperty(record);
            Types::addProperty(record, "text", &ESM::Book::mText);
            Types::addProperty(record, "enchant", &ESM::Book::mEnchant);
            Types::addProperty(record, "value", &ESM::Book::mData, &ESM::Book::BKDTstruct::mValue);
            Types::addProperty(record, "weight", &ESM::Book::mData, &ESM::Book::BKDTstruct::mWeight);

            const auto getScroll
                = [](const T& rec) -> bool { return Types::RecordType<T>::asRecord(rec).mData.mIsScroll; };
            const auto getEnchant
                = [](const T& rec) -> float { return Types::RecordType<T>::asRecord(rec).mData.mEnchant * 0.1f; };
            const auto getSkill = [](const T& rec) -> ESM::RefId {
                return ESM::Skill::indexToRefId(Types::RecordType<T>::asRecord(rec).mData.mSkillId);
            };
            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["isScroll"] = sol::property(
                    std::move(getScroll), [](T& rec, bool value) { rec.find().mData.mIsScroll = value; });
                record["enchantCapacity"] = sol::property(std::move(getEnchant), [](T& rec, Misc::FiniteFloat value) {
                    rec.find().mData.mEnchant = static_cast<int32_t>(std::round(value.mValue * 10));
                });
                record["skill"] = sol::property(std::move(getSkill), [](T& rec, std::string_view value) {
                    rec.find().mData.mSkillId = ESM::Skill::refIdToIndex(ESM::RefId::deserializeText(value));
                });
            }
            else
            {
                record["isScroll"] = sol::readonly_property(std::move(getScroll));
                record["enchantCapacity"] = sol::readonly_property(std::move(getEnchant));
                record["skill"] = sol::readonly_property(std::move(getSkill));
            }
        }
    }

    // Populates a book struct from a Lua table.
    ESM::Book tableToBook(const sol::table& rec)
    {
        auto book = Types::initFromTemplate<ESM::Book>(rec);
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
            book.mData.mEnchant
                = static_cast<int32_t>(std::round(rec["enchantCapacity"].get<Misc::FiniteFloat>() * 10));
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            book.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["weight"] != sol::nil)
            book.mData.mWeight = rec["weight"].get<Misc::FiniteFloat>();
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

    void addMutableBookType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Book>>(lua, "ESM3_MutableBook");
    }

    void addBookBindings(sol::table book, const Context& context)
    {
        sol::state_view lua = context.sol();
        // types.book.SKILL is deprecated (core.SKILL should be used instead)
        // TODO: Remove book.SKILL after branching 0.49
        sol::table skill(lua, sol::create);
        book["SKILL"] = LuaUtil::makeStrictReadOnly(skill);
        book["createRecordDraft"] = tableToBook;
        for (int id = 0; id < ESM::Skill::Length; ++id)
        {
            std::string skillName = ESM::Skill::indexToRefId(id).serializeText();
            skill[skillName] = skillName;
        }

        addRecordFunctionBinding<ESM::Book>(book, context);
        addUserType<ESM::Book>(lua, "ESM3_Book");
    }
}