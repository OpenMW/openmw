#include "types.hpp"

#include <components/esm3/loadcont.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwworld/class.hpp"

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
        container["content"] = sol::overload([](const LObject& o) { return Inventory<LObject>{ o }; },
            [](const GObject& o) { return Inventory<GObject>{ o }; });
        container["inventory"] = container["content"];
        container["getEncumbrance"] = [](const Object& obj) -> float {
            const MWWorld::Ptr& ptr = containerPtr(obj);
            return ptr.getClass().getEncumbrance(ptr);
        };
        container["encumbrance"] = container["getEncumbrance"]; // for compatibility; should be removed later
        container["getCapacity"] = [](const Object& obj) -> float {
            const MWWorld::Ptr& ptr = containerPtr(obj);
            return ptr.getClass().getCapacity(ptr);
        };
        container["capacity"] = container["getCapacity"]; // for compatibility; should be removed later

        addRecordFunctionBinding<ESM::Container>(container, context);

        sol::usertype<ESM::Container> record = context.mLua->sol().new_usertype<ESM::Container>("ESM3_Container");
        record[sol::meta_function::to_string] = [](const ESM::Container& rec) -> std::string {
            return "ESM3_Container[" + rec.mId.toDebugString() + "]";
        };
        record["id"]
            = sol::readonly_property([](const ESM::Container& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Container& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Container& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel);
        });
        record["mwscript"] = sol::readonly_property([](const ESM::Container& rec) -> sol::optional<std::string> {
            return LuaUtil::serializeRefId(rec.mScript);
        });
        record["weight"] = sol::readonly_property([](const ESM::Container& rec) -> float { return rec.mWeight; });
        record["isOrganic"] = sol::readonly_property(
            [](const ESM::Container& rec) -> bool { return rec.mFlags & ESM::Container::Organic; });
        record["isRespawning"] = sol::readonly_property(
            [](const ESM::Container& rec) -> bool { return rec.mFlags & ESM::Container::Respawn; });
    }
}
