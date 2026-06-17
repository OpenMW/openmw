#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadlock.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Lockpick> : std::false_type
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
                = [](const T& rec) -> std::string { return "ESM3_Lockpick[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Lockpick::mName);
            Types::addModelProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Lockpick::mScript);
            Types::addIconProperty(record);
            Types::addProperty(record, "maxCondition", &ESM::Lockpick::mData, &ESM::Lockpick::Data::mUses);
            Types::addProperty(record, "value", &ESM::Lockpick::mData, &ESM::Lockpick::Data::mValue);
            Types::addProperty(record, "weight", &ESM::Lockpick::mData, &ESM::Lockpick::Data::mWeight);
            Types::addProperty(record, "quality", &ESM::Lockpick::mData, &ESM::Lockpick::Data::mQuality);
        }
    }

    ESM::Lockpick tableToLockpick(const sol::table& rec)
    {
        auto pick = Types::initFromTemplate<ESM::Lockpick>(rec);
        if (rec["name"] != sol::nil)
            pick.mName = rec["name"];
        if (rec["model"] != sol::nil)
            pick.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            pick.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["icon"] != sol::nil)
            pick.mIcon = rec["icon"];
        if (rec["maxCondition"] != sol::nil)
            pick.mData.mUses = rec["maxCondition"];
        if (rec["value"] != sol::nil)
            pick.mData.mValue = rec["value"];
        if (rec["weight"] != sol::nil)
            pick.mData.mWeight = rec["weight"].get<Misc::FiniteFloat>();
        if (rec["quality"] != sol::nil)
            pick.mData.mQuality = rec["quality"].get<Misc::FiniteFloat>();
        return pick;
    }

    void addMutableLockpickType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Lockpick>>(lua, "ESM3_MutableLockpick");
    }

    void addLockpickBindings(sol::table lockpick, const Context& context)
    {
        addRecordFunctionBinding<ESM::Lockpick>(lockpick, context);

        sol::state_view lua = context.sol();
        addUserType<ESM::Lockpick>(lua, "ESM3_Lockpick");
    }
}