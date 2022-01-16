#include "luabindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/queries/luabindings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwphysics/raycasting.hpp"

#include "worldview.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWPhysics::RayCastingResult> : std::false_type {};
}

namespace MWLua
{
    sol::table initNearbyPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        WorldView* worldView = context.mWorldView;

        sol::usertype<MWPhysics::RayCastingResult> rayResult =
            context.mLua->sol().new_usertype<MWPhysics::RayCastingResult>("RayCastingResult");
        rayResult["hit"] = sol::readonly_property([](const MWPhysics::RayCastingResult& r) { return r.mHit; });
        rayResult["hitPos"] = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<osg::Vec3f>
        {
            if (r.mHit)
                return r.mHitPos;
            else
                return sol::nullopt;
        });
        rayResult["hitNormal"] = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<osg::Vec3f>
        {
            if (r.mHit)
                return r.mHitNormal;
            else
                return sol::nullopt;
        });
        rayResult["hitObject"] = sol::readonly_property([worldView](const MWPhysics::RayCastingResult& r) -> sol::optional<LObject>
        {
            if (r.mHitObject.isEmpty())
                return sol::nullopt;
            else
                return LObject(getId(r.mHitObject), worldView->getObjectRegistry());
        });

        api["COLLISION_TYPE"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, MWPhysics::CollisionType>({
            {"World", MWPhysics::CollisionType_World},
            {"Door", MWPhysics::CollisionType_Door},
            {"Actor", MWPhysics::CollisionType_Actor},
            {"HeightMap", MWPhysics::CollisionType_HeightMap},
            {"Projectile", MWPhysics::CollisionType_Projectile},
            {"Water", MWPhysics::CollisionType_Water},
            {"Default", MWPhysics::CollisionType_Default}
        }));

        api["castRay"] = [](const osg::Vec3f& from, const osg::Vec3f& to, sol::optional<sol::table> options)
        {
            MWWorld::Ptr ignore;
            int collisionType = MWPhysics::CollisionType_Default;
            float radius = 0;
            if (options)
            {
                sol::optional<LObject> ignoreObj = options->get<sol::optional<LObject>>("ignore");
                if (ignoreObj) ignore = ignoreObj->ptr();
                collisionType = options->get<sol::optional<int>>("collisionType").value_or(collisionType);
                radius = options->get<sol::optional<float>>("radius").value_or(0);
            }
            const MWPhysics::RayCastingInterface* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();
            if (radius <= 0)
                return rayCasting->castRay(from, to, ignore, std::vector<MWWorld::Ptr>(), collisionType);
            else
            {
                if (!ignore.isEmpty()) throw std::logic_error("Currently castRay doesn't support `ignore` when radius > 0");
                return rayCasting->castSphere(from, to, radius, collisionType);
            }
        };
        // TODO: async raycasting
        /*api["asyncCastRay"] = [luaManager = context.mLuaManager](
            const Callback& luaCallback, const osg::Vec3f& from, const osg::Vec3f& to, sol::optional<sol::table> options)
        {
            std::function<void(MWPhysics::RayCastingResult)> callback =
                luaManager->wrapLuaCallback<MWPhysics::RayCastingResult>(luaCallback);
            MWPhysics::RayCastingInterface* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();

            // Handle options the same way as in `castRay`.

            // NOTE: `callback` is not thread safe. If MWPhysics works in separate thread, it must put results to a queue
            //       and use this callback from the main thread at the beginning of the next frame processing.
            rayCasting->asyncCastRay(callback, from, to, ignore, std::vector<MWWorld::Ptr>(), collisionType);
        };*/

        api["activators"] = LObjectList{worldView->getActivatorsInScene()};
        api["actors"] = LObjectList{worldView->getActorsInScene()};
        api["containers"] = LObjectList{worldView->getContainersInScene()};
        api["doors"] = LObjectList{worldView->getDoorsInScene()};
        api["items"] = LObjectList{worldView->getItemsInScene()};
        api["selectObjects"] = [context](const Queries::Query& query)
        {
            ObjectIdList list;
            WorldView* worldView = context.mWorldView;
            if (query.mQueryType == "activators")
                list = worldView->getActivatorsInScene();
            else if (query.mQueryType == "actors")
                list = worldView->getActorsInScene();
            else if (query.mQueryType == "containers")
                list = worldView->getContainersInScene();
            else if (query.mQueryType == "doors")
                list = worldView->getDoorsInScene();
            else if (query.mQueryType == "items")
                list = worldView->getItemsInScene();
            return LObjectList{selectObjectsFromList(query, list, context)};
            // TODO: Maybe use sqlite
            // return LObjectList{worldView->selectObjects(query, true)};
        };
        return LuaUtil::makeReadOnly(api);
    }
}
