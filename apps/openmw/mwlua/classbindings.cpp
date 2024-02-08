#include <components/esm3/loadclas.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "classbindings.hpp"
#include "luamanagerimp.hpp"
#include "stats.hpp"
#include "types/types.hpp"

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

    sol::table initClassRecordBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table classes(context.mLua->sol(), sol::create);
        addRecordFunctionBinding<ESM::Class>(classes, context);

        auto classT = lua.new_usertype<ESM::Class>("ESM3_Class");
        classT[sol::meta_function::to_string]
            = [](const ESM::Class& rec) -> std::string { return "ESM3_Class[" + rec.mId.toDebugString() + "]"; };
        classT["id"] = sol::readonly_property([](const ESM::Class& rec) { return rec.mId.serializeText(); });
        classT["name"] = sol::readonly_property([](const ESM::Class& rec) -> std::string_view { return rec.mName; });
        classT["description"]
            = sol::readonly_property([](const ESM::Class& rec) -> std::string_view { return rec.mDescription; });

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
            return ESM::Class::specializationIndexToLuaId.at(rec.mData.mSpecialization);
        });
        classT["isPlayable"]
            = sol::readonly_property([](const ESM::Class& rec) -> bool { return rec.mData.mIsPlayable; });
        return LuaUtil::makeReadOnly(classes);
    }
}
