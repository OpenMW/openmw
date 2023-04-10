#include "types.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Creature> : std::false_type
    {
    };
}

namespace MWLua
{
    void addCreatureBindings(sol::table creature, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Creature>(creature, context);

        sol::usertype<ESM::Creature> record = context.mLua->sol().new_usertype<ESM::Creature>("ESM3_Creature");
        record[sol::meta_function::to_string]
            = [](const ESM::Creature& rec) { return "ESM3_Creature[" + rec.mId.toDebugString() + "]"; };
        record["name"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Creature& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return rec.mScript.serializeText(); });
        record["baseCreature"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return rec.mOriginal.serializeText(); });
    }
}
