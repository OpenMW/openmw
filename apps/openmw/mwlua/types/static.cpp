#include "types.hpp"

#include <components/esm3/loadstat.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Static> : std::false_type
    {
    };
}

namespace MWLua
{
    void addStaticBindings(sol::table stat, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        const MWWorld::Store<ESM::Static>* store
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>();
        stat["record"]
            = sol::overload([](const Object& obj) -> const ESM::Static* { return obj.ptr().get<ESM::Static>()->mBase; },
                [store](const std::string& recordId) -> const ESM::Static* {
                    return store->find(ESM::RefId::stringRefId(recordId));
                });
        sol::usertype<ESM::Static> record = context.mLua->sol().new_usertype<ESM::Static>("ESM3_Static");
        record[sol::meta_function::to_string]
            = [](const ESM::Static& rec) -> std::string { return "ESM3_Static[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Static& rec) -> std::string { return rec.mId.getRefIdString(); });
        record["model"] = sol::readonly_property([vfs](const ESM::Static& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
    }
}
