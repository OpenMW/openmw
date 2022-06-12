#include "types.hpp"

#include <components/esm3/loadlock.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Lockpick> : std::false_type {};
}

namespace MWLua
{
    void addLockpickBindings(sol::table lockpick, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        const MWWorld::Store<ESM::Lockpick>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Lockpick>();
        lockpick["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Lockpick* { return obj.ptr().get<ESM::Lockpick>()->mBase;},
            [store](const std::string& recordId) -> const ESM::Lockpick* { return store->find(recordId);});
        sol::usertype<ESM::Lockpick> record = context.mLua->sol().new_usertype<ESM::Lockpick>("ESM3_Lockpick");
        record[sol::meta_function::to_string] = [](const ESM::Lockpick& rec) { return "ESM3_Lockpick[" + rec.mId + "]";};
        record["id"] = sol::readonly_property([](const ESM::Lockpick& rec) -> std::string { return rec.mId;});
        record["name"] = sol::readonly_property([](const ESM::Lockpick& rec) -> std::string { return rec.mName;});
        record["model"] = sol::readonly_property([vfs](const ESM::Lockpick& rec) -> std::string
        {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"] = sol::readonly_property([](const ESM::Lockpick& rec) -> std::string { return rec.mScript;});
        record["icon"] = sol::readonly_property([vfs](const ESM::Lockpick& rec) -> std::string
        {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["maxCondition"] = sol::readonly_property([](const ESM::Lockpick& rec) -> int { return rec.mData.mUses;});
        record["value"] = sol::readonly_property([](const ESM::Lockpick& rec) -> int { return rec.mData.mValue;});
        record["weight"] = sol::readonly_property([](const ESM::Lockpick& rec) -> float { return rec.mData.mWeight;});
        record["quality"] = sol::readonly_property([](const ESM::Lockpick& rec) -> float { return rec.mData.mQuality;});
    }
}
