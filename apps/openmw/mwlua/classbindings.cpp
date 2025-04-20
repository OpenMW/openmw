#include "classbindings.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/lua/luastate.hpp>

#include "idcollectionbindings.hpp"
#include "recordstore.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Class> : std::false_type
    {
    };
}

namespace MWLua
{

    sol::table initClassRecordBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table classes(lua, sol::create);
        addRecordFunctionBinding<ESM::Class>(classes, context);

        auto classT = lua.new_usertype<ESM::Class>("ESM3_Class");
        classT[sol::meta_function::to_string]
            = [](const ESM::Class& rec) -> std::string { return "ESM3_Class[" + rec.mId.toDebugString() + "]"; };
        classT["id"] = sol::readonly_property([](const ESM::Class& rec) { return rec.mId.serializeText(); });
        classT["name"] = sol::readonly_property([](const ESM::Class& rec) -> std::string_view { return rec.mName; });
        classT["description"]
            = sol::readonly_property([](const ESM::Class& rec) -> std::string_view { return rec.mDescription; });

        classT["attributes"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
            return createReadOnlyRefIdTable(lua, rec.mData.mAttribute, ESM::Attribute::indexToRefId);
        });
        classT["majorSkills"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
            return createReadOnlyRefIdTable(
                lua, rec.mData.mSkills, [](const auto& pair) { return ESM::Skill::indexToRefId(pair[1]); });
        });
        classT["minorSkills"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
            return createReadOnlyRefIdTable(
                lua, rec.mData.mSkills, [](const auto& pair) { return ESM::Skill::indexToRefId(pair[0]); });
        });

        classT["specialization"] = sol::readonly_property([](const ESM::Class& rec) -> std::string_view {
            return ESM::Class::specializationIndexToLuaId.at(rec.mData.mSpecialization);
        });
        classT["isPlayable"]
            = sol::readonly_property([](const ESM::Class& rec) -> bool { return rec.mData.mIsPlayable; });
        return LuaUtil::makeReadOnly(classes);
    }
}
