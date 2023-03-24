#include "types.hpp"

#include <components/esm3/loadcont.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/class.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Container> : std::false_type
    {
    };
}

namespace MWLua
{

    static const MWWorld::Ptr& containerPtr(const Object& o)
    {
        return verifyType(ESM::REC_CONT, o.ptr());
    }

    void addContainerBindings(sol::table container, const Context& context)
    {
        container["content"] = sol::overload(
            [](const LObject& o) {
                containerPtr(o);
                return Inventory<LObject>{ o };
            },
            [](const GObject& o) {
                containerPtr(o);
                return Inventory<GObject>{ o };
            });
        container["encumbrance"] = [](const Object& obj) -> float {
            const MWWorld::Ptr& ptr = containerPtr(obj);
            return ptr.getClass().getEncumbrance(ptr);
        };
        container["capacity"] = [](const Object& obj) -> float {
            const MWWorld::Ptr& ptr = containerPtr(obj);
            return ptr.getClass().getCapacity(ptr);
        };

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Container>(container);

        sol::usertype<ESM::Container> record = context.mLua->sol().new_usertype<ESM::Container>("ESM3_Container");
        record[sol::meta_function::to_string] = [](const ESM::Container& rec) -> std::string {
            return "ESM3_Container[" + rec.mId.toDebugString() + "]";
        };
        record["id"]
            = sol::readonly_property([](const ESM::Container& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Container& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Container& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Container& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Container& rec) -> float { return rec.mWeight; });
    }
}
