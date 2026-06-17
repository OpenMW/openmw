#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadrepa.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Repair> : std::false_type
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
                = [](const T& rec) -> std::string { return "ESM3_Repair[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Repair::mName);
            Types::addModelProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Repair::mScript);
            Types::addIconProperty(record);
            Types::addProperty(record, "maxCondition", &ESM::Repair::mData, &ESM::Repair::Data::mUses);
            Types::addProperty(record, "value", &ESM::Repair::mData, &ESM::Repair::Data::mValue);
            Types::addProperty(record, "weight", &ESM::Repair::mData, &ESM::Repair::Data::mWeight);
            Types::addProperty(record, "quality", &ESM::Repair::mData, &ESM::Repair::Data::mQuality);
        }
    }

    ESM::Repair tableToRepair(const sol::table& rec)
    {
        auto repair = Types::initFromTemplate<ESM::Repair>(rec);
        if (rec["name"] != sol::nil)
            repair.mName = rec["name"];
        if (rec["model"] != sol::nil)
            repair.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            repair.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["icon"] != sol::nil)
            repair.mIcon = rec["icon"];
        if (rec["maxCondition"] != sol::nil)
            repair.mData.mUses = rec["maxCondition"];
        if (rec["value"] != sol::nil)
            repair.mData.mValue = rec["value"];
        if (rec["weight"] != sol::nil)
            repair.mData.mWeight = rec["weight"].get<Misc::FiniteFloat>();
        if (rec["quality"] != sol::nil)
            repair.mData.mQuality = rec["quality"].get<Misc::FiniteFloat>();
        return repair;
    }

    void addMutableRepairType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Repair>>(lua, "ESM3_MutableRepair");
    }

    void addRepairBindings(sol::table repair, const Context& context)
    {
        addRecordFunctionBinding<ESM::Repair>(repair, context);

        sol::state_view lua = context.sol();
        addUserType<ESM::Repair>(lua, "ESM3_Repair");
    }
}