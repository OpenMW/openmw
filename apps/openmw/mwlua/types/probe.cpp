#include "types.hpp"

#include <components/esm3/loadprob.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Probe> : std::false_type {};
}

namespace MWLua
{
    void addProbeBindings(sol::table probe, const Context& context)
    {
        const MWWorld::Store<ESM::Probe>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Probe>();
        probe["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Probe* { return obj.ptr().get<ESM::Probe>()->mBase;},
            [store](const std::string& recordId) -> const ESM::Probe* { return store->find(recordId);});
        sol::usertype<ESM::Probe> record = context.mLua->sol().new_usertype<ESM::Probe>("ESM3_Probe");
        record[sol::meta_function::to_string] = [](const ESM::Probe& rec) { return "ESM3_Probe[" + rec.mId + "]";};
        record["id"] = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mId;});
        record["name"] = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mName;});
        record["model"] = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mModel;});
        record["mwscript"] = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mScript;});
        record["icon"] = sol::readonly_property([](const ESM::Probe& rec) -> std::string { return rec.mIcon;});
        record["maxCondition"] = sol::readonly_property([](const ESM::Probe& rec) -> int { return rec.mData.mUses;});
        record["value"] = sol::readonly_property([](const ESM::Probe& rec) -> int { return rec.mData.mValue;});
        record["weight"] = sol::readonly_property([](const ESM::Probe& rec) -> float { return rec.mData.mWeight;});
        record["quality"] = sol::readonly_property([](const ESM::Probe& rec) -> float { return rec.mData.mQuality;});
    }
}
