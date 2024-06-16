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

namespace
{
    void setRecordFlag(const sol::table& rec, const std::string& key, int flag, ESM::Light& record)
    {
        if (auto luaFlag = rec[key]; luaFlag != sol::nil)
        {
            if (luaFlag)
            {
                record.mData.mFlags |= flag;
            }
            else
            {
                record.mData.mFlags &= ~flag;
            }
        }
    }
    // Populates a light struct from a Lua table.
    ESM::Light tableToLight(const sol::table& rec)
    {
        ESM::Light light;
        if (rec["template"] != sol::nil)
            light = LuaUtil::cast<ESM::Light>(rec["template"]);
        else
            light.blank();
        if (rec["name"] != sol::nil)
            light.mName = rec["name"];
        if (rec["model"] != sol::nil)
            light.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            light.mIcon = rec["icon"];
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            light.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["weight"] != sol::nil)
            light.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            light.mData.mValue = rec["value"];
        if (rec["duration"] != sol::nil)
            light.mData.mTime = rec["duration"];
        if (rec["radius"] != sol::nil)
            light.mData.mRadius = rec["radius"];
        if (rec["color"] != sol::nil)
            light.mData.mColor = rec["color"];
        setRecordFlag(rec, "isCarriable", ESM::Light::Carry, light);
        setRecordFlag(rec, "isDynamic", ESM::Light::Dynamic, light);
        setRecordFlag(rec, "isFire", ESM::Light::Fire, light);
        setRecordFlag(rec, "isFlicker", ESM::Light::Flicker, light);
        setRecordFlag(rec, "isFlickerSlow", ESM::Light::FlickerSlow, light);
        setRecordFlag(rec, "isNegative", ESM::Light::Negative, light);
        setRecordFlag(rec, "isOffByDefault", ESM::Light::OffDefault, light);
        setRecordFlag(rec, "isPulse", ESM::Light::Pulse, light);
        setRecordFlag(rec, "isPulseSlow", ESM::Light::PulseSlow, light);

        return light;
    }
}

namespace MWLua
{
    void addLightBindings(sol::table light, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Light>(light, context);
        light["createRecordDraft"] = tableToLight;

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
        record["isDynamic"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::Dynamic; });
        record["isFire"]
            = sol::readonly_property([](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::Fire; });
        record["isFlicker"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::Flicker; });
        record["isFlickerSlow"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::FlickerSlow; });
        record["isNegative"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::Negative; });
        record["isOffByDefault"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::OffDefault; });
        record["isPulse"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::Pulse; });
        record["isPulseSlow"] = sol::readonly_property(
            [](const ESM::Light& rec) -> bool { return rec.mData.mFlags & ESM::Light::PulseSlow; });
    }
}
