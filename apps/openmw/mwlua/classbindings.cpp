#include "classbindings.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "luamanagerimp.hpp"

namespace
{
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Class> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::Store<ESM::Class>> : std::false_type
    {
    };
}

namespace MWLua
{
    using classStore = MWWorld::Store<ESM::Class>;

    void initCoreClassBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::usertype<classStore> classStoreT = lua.new_usertype<classStore>("ESM3_classStore");
        classStoreT[sol::meta_function::to_string] = [](const classStore& store) {
            return "ESM3_classStore{" + std::to_string(store.getSize()) + " classes}";
        };
        classStoreT[sol::meta_function::length] = [](const classStore& store) { return store.getSize(); };
        classStoreT[sol::meta_function::index] = sol::overload(
            [](const classStore& store, size_t index) -> const ESM::Class* {
                if (index == 0 || index > store.getSize())
                    return nullptr;
                return store.at(index - 1);
            },
            [](const classStore& store, std::string_view classId) -> const ESM::Class* {
                return store.search(ESM::RefId::deserializeText(classId));
            });
        classStoreT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        classStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        // class record
        auto classT = lua.new_usertype<ESM::Class>("ESM3_Class");
        classT[sol::meta_function::to_string]
            = [](const ESM::Class& rec) -> std::string { return "ESM3_Class[" + rec.mId.toDebugString() + "]"; };
        classT["id"] = sol::readonly_property([](const ESM::Class& rec) { return rec.mId.serializeText(); });
        classT["name"] = sol::readonly_property([](const ESM::Class& rec) -> std::string_view { return rec.mName; });
        classT["description"]
            = sol::readonly_property([](const ESM::Class& rec) -> std::string_view { return rec.mDescription; });
        classT["majorSkills"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
            sol::table res(lua, sol::create);
            auto skills = rec.mData.mSkills;

            for (size_t i = 0; i < skills.size(); ++i)
            {
                ESM::RefId skillId = ESM::Skill::indexToRefId(skills[i][1]);
                res[i + 1] = skillId.serializeText();
            }

            return res;
        });
        classT["attributes"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
            sol::table res(lua, sol::create);
            auto attribute = rec.mData.mAttribute;

            for (size_t i = 0; i < attribute.size(); ++i)
            {
                ESM::RefId attributeId = ESM::Attribute::indexToRefId(attribute[i]);
                res[i + 1] = attributeId.serializeText();
            }

            return res;
        });
        classT["minorSkills"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
            sol::table res(lua, sol::create);
            auto skills = rec.mData.mSkills;

            for (size_t i = 0; i < skills.size(); ++i)
            {
                ESM::RefId skillId = ESM::Skill::indexToRefId(skills[i][0]);
                res[i + 1] = skillId.serializeText();
            }

            return res;
        });
        classT["specialization"] = sol::readonly_property([](const ESM::Class& rec) -> std::string_view {
            if (rec.mData.mSpecialization == ESM::Class::Stealth)
                return "stealth";
            else if (rec.mData.mSpecialization == ESM::Class::Magic)
                return "magic";
            else
                return "combat";
        });
        classT["isPlayable"]
            = sol::readonly_property([](const ESM::Class& rec) -> bool { return rec.mData.mIsPlayable; });
    }
}
