#include "types.hpp"

#include <components/esm3/loadalch.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Potion> : std::false_type {};
}

namespace MWLua
{
    void addPotionBindings(sol::table potion, const Context& context)
    {
        const MWWorld::Store<ESM::Potion>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Potion>();
        potion["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Potion* { return obj.ptr().get<ESM::Potion>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Potion* { return store->find(recordId); });

        sol::usertype<ESM::Potion> record = context.mLua->sol().new_usertype<ESM::Potion>("ESM3_Potion");
        record[sol::meta_function::to_string] = [](const ESM::Potion& rec) { return "ESM3_Potion[" + rec.mId + "]"; };
        record["id"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mModel; });
        record["icon"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mIcon; });
        record["mwscript"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mScript; });
        record["weight"] = sol::readonly_property([](const ESM::Potion& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Potion& rec) -> int { return rec.mData.mValue; });
    }
}
