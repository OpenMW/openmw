#include "types.hpp"

#include <components/esm3/loadacti.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Activator> : std::false_type
    {
    };
}
namespace
{
    // Populates a activator struct from a Lua table.
    ESM::Activator tableToActivator(const sol::table& rec)
    {
        ESM::Activator activator;
        activator.mName = rec["name"];
        activator.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        std::string_view scriptId = rec["mwscript"].get<std::string_view>();
        activator.mScript = ESM::RefId::deserializeText(scriptId);
        activator.mRecordFlags = 0;
        return activator;
    }
}

namespace MWLua
{
    void addActivatorBindings(sol::table activator, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        activator["createRecordDraft"] = tableToActivator;
        addRecordFunctionBinding<ESM::Activator>(activator, context);

        sol::usertype<ESM::Activator> record = context.mLua->sol().new_usertype<ESM::Activator>("ESM3_Activator");
        record[sol::meta_function::to_string]
            = [](const ESM::Activator& rec) { return "ESM3_Activator[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Activator& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Activator& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Activator& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Activator& rec) -> std::string { return rec.mScript.serializeText(); });
    }
}
