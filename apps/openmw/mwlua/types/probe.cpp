#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadprob.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Probe> : std::false_type
    {
    };
}

namespace MWLua
{
    void addProbeBindings(sol::table probe, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Probe>(probe, context);

        sol::usertype<ESM::Probe> record = context.sol().new_usertype<ESM::Probe>("ESM3_Probe");
        record[sol::meta_function::to_string]
            = [](const ESM::Probe& rec) { return "ESM3_Probe[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["mwscript"] = sol::readonly_property([](const ESM::Probe& rec) -> ESM::RefId { return rec.mScript; });
        record["icon"] = sol::readonly_property([vfs](const ESM::Probe& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["maxCondition"] = sol::readonly_property([](const ESM::Probe& rec) -> int { return rec.mData.mUses; });
        record["value"] = sol::readonly_property([](const ESM::Probe& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Probe& rec) -> float { return rec.mData.mWeight; });
        record["quality"] = sol::readonly_property([](const ESM::Probe& rec) -> float { return rec.mData.mQuality; });
    }
}