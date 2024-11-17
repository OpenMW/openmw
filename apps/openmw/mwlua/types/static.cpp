#include "types.hpp"

#include "modelproperty.hpp"

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
    void addStaticBindings(sol::table stat, const Context& context)
    {
        addRecordFunctionBinding<ESM::Static>(stat, context);

        sol::usertype<ESM::Static> record = context.sol().new_usertype<ESM::Static>("ESM3_Static");
        record[sol::meta_function::to_string]
            = [](const ESM::Static& rec) -> std::string { return "ESM3_Static[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Static& rec) -> std::string { return rec.mId.serializeText(); });
        addModelProperty(record);
    }
}
