#include "types.hpp"

#include <components/esm3/loadmisc.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Miscellaneous> : std::false_type {};
}

namespace MWLua
{
    void addMiscellaneousBindings(sol::table miscellaneous, const Context& context)
    {
        const MWWorld::Store<ESM::Miscellaneous>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Miscellaneous>();
        miscellaneous["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Miscellaneous* { return obj.ptr().get<ESM::Miscellaneous>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Miscellaneous* { return store->find(recordId); });
        sol::usertype<ESM::Miscellaneous> record = context.mLua->sol().new_usertype<ESM::Miscellaneous>("ESM3_Miscellaneous");
        record[sol::meta_function::to_string] = [](const ESM::Miscellaneous& rec) { return "ESM3_Miscellaneous[" + rec.mId + "]"; };
        record["id"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> std::string { return rec.mModel; });
        record["mwscript"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> std::string { return rec.mScript; });
        record["icon"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> std::string { return rec.mIcon; });
        record["isKey"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> bool { return rec.mData.mIsKey; });
        record["value"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Miscellaneous& rec) -> float { return rec.mData.mWeight; });
    }
}
