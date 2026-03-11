#include "types.hpp"

#include "../contentbindings.hpp"
#include "usertypeutil.hpp"

#include <components/esm3/loadstat.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Static> : std::false_type
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
                = [](const T& rec) -> std::string { return "ESM3_Static[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addModelProperty(record);
        }
    }

    ESM::Static tableToStatic(const sol::table& rec)
    {
        auto stat = Types::initFromTemplate<ESM::Static>(rec);

        if (rec["model"] != sol::nil)
            stat.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());

        return stat;
    }

    void addMutableStaticType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Static>>(lua, "ESM3_MutableStatic");
    }

    void addStaticBindings(sol::table stat, const Context& context)
    {
        stat["createRecordDraft"] = tableToStatic;
        addRecordFunctionBinding<ESM::Static>(stat, context);
        sol::state_view lua = context.sol();
        addUserType<ESM::Static>(lua, "ESM3_Static");
    }
}
