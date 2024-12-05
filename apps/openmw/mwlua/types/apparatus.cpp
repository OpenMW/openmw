#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadappa.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Apparatus> : std::false_type
    {
    };
}

namespace MWLua
{
    void addApparatusBindings(sol::table apparatus, const Context& context)
    {
        sol::state_view lua = context.sol();
        apparatus["TYPE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            {
                { "MortarPestle", ESM::Apparatus::MortarPestle },
                { "Alembic", ESM::Apparatus::Alembic },
                { "Calcinator", ESM::Apparatus::Calcinator },
                { "Retort", ESM::Apparatus::Retort },
            }));

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Apparatus>(apparatus, context);

        sol::usertype<ESM::Apparatus> record = lua.new_usertype<ESM::Apparatus>("ESM3_Apparatus");
        record[sol::meta_function::to_string]
            = [](const ESM::Apparatus& rec) { return "ESM3_Apparatus[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Apparatus& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Apparatus& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["mwscript"] = sol::readonly_property([](const ESM::Apparatus& rec) -> sol::optional<std::string> {
            return LuaUtil::serializeRefId(rec.mScript);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Apparatus& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["type"] = sol::readonly_property([](const ESM::Apparatus& rec) -> int { return rec.mData.mType; });
        record["value"] = sol::readonly_property([](const ESM::Apparatus& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Apparatus& rec) -> float { return rec.mData.mWeight; });
        record["quality"]
            = sol::readonly_property([](const ESM::Apparatus& rec) -> float { return rec.mData.mQuality; });
    }
}
