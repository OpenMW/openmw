#include "types.hpp"

#include "../contentbindings.hpp"
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
    ESM::Static tableToStatic(const sol::table& rec)
    {
        ESM::Static stat;
        if (rec["template"] != sol::nil)
        {
            if (rec["template"].is<MutableRecord<ESM::Static>>())
                stat = rec["template"].get<MutableRecord<ESM::Static>>().find();
            else
                stat = LuaUtil::cast<ESM::Static>(rec["template"]);
        }
        else
            stat.blank();

        if (rec["model"] != sol::nil)
            stat.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());

        return stat;
    }

    void addStaticBindings(sol::table stat, const Context& context)
    {
        stat["createRecordDraft"] = tableToStatic;
        addRecordFunctionBinding<ESM::Static>(stat, context);

        sol::usertype<ESM::Static> record = context.sol().new_usertype<ESM::Static>("ESM3_Static");
        record[sol::meta_function::to_string]
            = [](const ESM::Static& rec) -> std::string { return "ESM3_Static[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Static& rec) -> std::string { return rec.mId.serializeText(); });
        addModelProperty(record);
    }
}
