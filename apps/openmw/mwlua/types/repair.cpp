#include "types.hpp"

#include <components/esm3/loadrepa.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Repair> : std::false_type {};
}

namespace MWLua
{
    void addRepairBindings(sol::table repair, const Context& context)
    {
        const MWWorld::Store<ESM::Repair>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Repair>();
        repair["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Repair* { return obj.ptr().get<ESM::Repair>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Repair* { return store->find(recordId); });
        sol::usertype<ESM::Repair> record = context.mLua->sol().new_usertype<ESM::Repair>("ESM3_Repair");
        record[sol::meta_function::to_string] = [](const ESM::Repair& rec) { return "ESM3_Repair[" + rec.mId + "]"; };
        record["id"] = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mModel; });
        record["mwscript"] = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mScript; });
        record["icon"] = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mIcon; });
        record["maxCondition"] = sol::readonly_property([](const ESM::Repair& rec) -> int { return rec.mData.mUses; });
        record["value"] = sol::readonly_property([](const ESM::Repair& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Repair& rec) -> float { return rec.mData.mWeight; });
        record["quality"] = sol::readonly_property([](const ESM::Repair& rec) -> float { return rec.mData.mQuality; });
    }
}
