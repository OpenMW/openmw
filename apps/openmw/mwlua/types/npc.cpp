#include "types.hpp"

#include <components/esm3/loadnpc.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../stats.hpp"
#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::NPC> : std::false_type {};
}

namespace MWLua
{
    void addNpcBindings(sol::table npc, const Context& context)
    {
        addNpcStatsBindings(npc, context);

        const MWWorld::Store<ESM::NPC>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::NPC>();
        npc["record"] = sol::overload(
            [](const Object& obj) -> const ESM::NPC* { return obj.ptr().get<ESM::NPC>()->mBase; },
            [store](const std::string& recordId) -> const ESM::NPC* { return store->find(recordId); });
        sol::usertype<ESM::NPC> record = context.mLua->sol().new_usertype<ESM::NPC>("ESM3_NPC");
        record[sol::meta_function::to_string] = [](const ESM::NPC& rec) { return "ESM3_NPC[" + rec.mId + "]"; };
        record["name"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mName; });
        record["race"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mRace; });
        record["class"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mClass; });
        record["mwscript"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mScript; });
        record["hair"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mHair; });
        record["head"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mHead; });
    }
}
