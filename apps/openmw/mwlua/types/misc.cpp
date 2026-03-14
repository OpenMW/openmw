#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Miscellaneous> : std::false_type
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
                = [](const T& rec) -> std::string { return "ESM3_Miscellaneous[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Miscellaneous::mName);
            Types::addModelProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Miscellaneous::mScript);
            Types::addIconProperty(record);
            Types::addProperty(record, "value", &ESM::Miscellaneous::mData, &ESM::Miscellaneous::MCDTstruct::mValue);
            Types::addProperty(record, "weight", &ESM::Miscellaneous::mData, &ESM::Miscellaneous::MCDTstruct::mWeight);
            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["isKey"] = sol::property(
                    [](const T& mutRec) -> bool { return mutRec.find().mData.mFlags & ESM::Miscellaneous::Key; },
                    [](T& mutRec, bool key) {
                        auto& recordValue = mutRec.find();
                        if (key)
                            recordValue.mData.mFlags |= ESM::Miscellaneous::Key;
                        else
                            recordValue.mData.mFlags &= ~ESM::Miscellaneous::Key;
                    });
            }
            else
            {
                record["isKey"] = sol::readonly_property(
                    [](const ESM::Miscellaneous& rec) -> bool { return rec.mData.mFlags & ESM::Miscellaneous::Key; });
            }
        }
    }

    // Populates a misc struct from a Lua table.
    ESM::Miscellaneous tableToMisc(const sol::table& rec)
    {
        auto misc = Types::initFromTemplate<ESM::Miscellaneous>(rec);
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

    void addMutableMiscType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Miscellaneous>>(lua, "ESM3_MutableMiscellaneous");
    }

    void addMiscellaneousBindings(sol::table miscellaneous, const Context& context)
    {
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
        miscellaneous["getSoul"]
            = [](const Object& object) -> ESM::RefId { return object.ptr().getCellRef().getSoul(); };
        miscellaneous["soul"] = miscellaneous["getSoul"]; // for compatibility; should be removed later

        sol::state_view lua = context.sol();
        addUserType<ESM::Miscellaneous>(lua, "ESM3_Miscellaneous");
    }
}