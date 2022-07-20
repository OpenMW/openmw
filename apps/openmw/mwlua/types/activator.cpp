#include "types.hpp"

#include <components/esm3/loadacti.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Activator> : std::false_type {};
}

namespace MWLua
{
    void addActivatorBindings(sol::table activator, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        const MWWorld::Store<ESM::Activator>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Activator>();
        activator["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Activator* { return obj.ptr().get<ESM::Activator>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Activator* { return store->find(recordId); });
        sol::usertype<ESM::Activator> record = context.mLua->sol().new_usertype<ESM::Activator>("ESM3_Activator");
        record[sol::meta_function::to_string] = [](const ESM::Activator& rec) { return "ESM3_Activator[" + rec.mId + "]"; };
        record["id"] = sol::readonly_property([](const ESM::Activator& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Activator& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Activator& rec) -> std::string
        {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"] = sol::readonly_property([](const ESM::Activator& rec) -> std::string { return rec.mScript; });
    }
}
