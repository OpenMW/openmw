#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Miscellaneous> : std::false_type
    {
    };
}

namespace
{
    // Populates a misc struct from a Lua table.
    ESM::Miscellaneous tableToMisc(const sol::table& rec)
    {
        ESM::Miscellaneous misc;
        if (rec["template"] != sol::nil)
            misc = LuaUtil::cast<ESM::Miscellaneous>(rec["template"]);
        else
            misc.blank();
        if (rec["name"] != sol::nil)
            misc.mName = rec["name"];
        if (rec["model"] != sol::nil)
            misc.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            misc.mIcon = rec["icon"];
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            misc.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["weight"] != sol::nil)
            misc.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            misc.mData.mValue = rec["value"];
        return misc;
    }
}

namespace MWLua
{
    void addMiscellaneousBindings(sol::table miscellaneous, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Miscellaneous>(miscellaneous, context);
        miscellaneous["createRecordDraft"] = tableToMisc;

        // Deprecated. Moved to itemData; should be removed later
        miscellaneous["setSoul"] = [](const GObject& object, std::string_view soulId) {
            ESM::RefId creature = ESM::RefId::deserializeText(soulId);
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

            if (!store.get<ESM::Creature>().search(creature))
            {
                // TODO: Add Support for NPC Souls
                throw std::runtime_error("Cannot use non-existent creature as a soul: " + std::string(soulId));
            }

            object.ptr().getCellRef().setSoul(creature);
        };
        miscellaneous["getSoul"] = [](const Object& object) -> sol::optional<std::string> {
            ESM::RefId soul = object.ptr().getCellRef().getSoul();
            return LuaUtil::serializeRefId(soul);
        };
        miscellaneous["soul"] = miscellaneous["getSoul"]; // for compatibility; should be removed later

        sol::usertype<ESM::Miscellaneous> record = context.sol().new_usertype<ESM::Miscellaneous>("ESM3_Miscellaneous");
        record[sol::meta_function::to_string]
            = [](const ESM::Miscellaneous& rec) { return "ESM3_Miscellaneous[" + rec.mId.toDebugString() + "]"; };
        record["id"] = sol::readonly_property(
            [](const ESM::Miscellaneous& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["mwscript"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> sol::optional<std::string> {
            return LuaUtil::serializeRefId(rec.mScript);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Miscellaneous& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["isKey"] = sol::readonly_property(
            [](const ESM::Miscellaneous& rec) -> bool { return rec.mData.mFlags & ESM::Miscellaneous::Key; });
        record["value"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> int { return rec.mData.mValue; });
        record["weight"]
            = sol::readonly_property([](const ESM::Miscellaneous& rec) -> float { return rec.mData.mWeight; });
    }
}
