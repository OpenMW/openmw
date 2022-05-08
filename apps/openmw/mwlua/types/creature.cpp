#include "types.hpp"

#include <components/esm3/loadcrea.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../stats.hpp"
#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Creature> : std::false_type {};
}

namespace MWLua
{
    void addCreatureBindings(sol::table creature, const Context& context)
    {
        const MWWorld::Store<ESM::Creature>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>();
        creature["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Creature* { return obj.ptr().get<ESM::Creature>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Creature* { return store->find(recordId); });
        sol::usertype<ESM::Creature> record = context.mLua->sol().new_usertype<ESM::Creature>("ESM3_Creature");
        record[sol::meta_function::to_string] = [](const ESM::Creature& rec) { return "ESM3_Creature[" + rec.mId + "]"; };
        record["name"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mModel; });
        record["mwscript"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mScript; });
        record["baseCreature"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mOriginal; });
    }
}
