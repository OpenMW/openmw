#include "types.hpp"

#include <components/esm3/loadligh.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Light> : std::false_type
    {
    };
}

namespace MWLua
{
    void addLightBindings(sol::table light, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Light>(light, context);

        sol::usertype<ESM::Light> record = context.mLua->sol().new_usertype<ESM::Light>("ESM3_Light");
        record[sol::meta_function::to_string]
            = [](const ESM::Light& rec) -> std::string { return "ESM3_Light[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Light& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Light& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property(
            [](const ESM::Light& rec) -> std::string { return Misc::ResourceHelpers::correctMeshPath(rec.mModel); });
        record["icon"] = sol::readonly_property([vfs](const ESM::Light& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["sound"]
            = sol::readonly_property([](const ESM::Light& rec) -> std::string { return rec.mSound.serializeText(); });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Light& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Light& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Light& rec) -> int { return rec.mData.mValue; });
        record["duration"] = sol::readonly_property([](const ESM::Light& rec) -> int { return rec.mData.mTime; });
        record["radius"] = sol::readonly_property([](const ESM::Light& rec) -> int { return rec.mData.mRadius; });
        record["color"] = sol::readonly_property([](const ESM::Light& rec) -> int { return rec.mData.mColor; });
        record["isCarriable"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::Carry; });
    }
}
