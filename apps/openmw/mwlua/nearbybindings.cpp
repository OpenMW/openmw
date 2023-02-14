#include "luabindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/navigatorutils.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwphysics/raycasting.hpp"

#include "luamanagerimp.hpp"
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

        api["COLLISION_TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, MWPhysics::CollisionType>({
            {"World", MWPhysics::CollisionType_World},
            {"Door", MWPhysics::CollisionType_Door},
            {"Actor", MWPhysics::CollisionType_Actor},
            {"HeightMap", MWPhysics::CollisionType_HeightMap},
            {"Projectile", MWPhysics::CollisionType_Projectile},
            {"Water", MWPhysics::CollisionType_Water},
            {"Default", MWPhysics::CollisionType_Default},
            {"AnyPhysical", MWPhysics::CollisionType_AnyPhysical},
            {"Camera", MWPhysics::CollisionType_CameraOnly},
            {"VisualOnly", MWPhysics::CollisionType_VisualOnly},
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
        api["castRenderingRay"] = [manager=context.mLuaManager](const osg::Vec3f& from, const osg::Vec3f& to)
        {
            if (!manager->isProcessingInputEvents())
            {
                throw std::logic_error("castRenderingRay can be used only in player scripts during processing of input events; "
                                       "use asyncCastRenderingRay instead.");
            }
            MWPhysics::RayCastingResult res;
            MWBase::Environment::get().getWorld()->castRenderingRay(res, from, to, false, false);
            return res;
        };
        api["asyncCastRenderingRay"] = [context](
                                           const sol::table& callback, const osg::Vec3f& from, const osg::Vec3f& to) {
            context.mLuaManager->addAction([context, callback = LuaUtil::Callback::fromLua(callback), from, to] {
                MWPhysics::RayCastingResult res;
                MWBase::Environment::get().getWorld()->castRenderingRay(res, from, to, false, false);
                context.mLuaManager->queueCallback(callback, sol::main_object(context.mLua->sol(), sol::in_place, res));
            });
        };

        api["activators"] = LObjectList{worldView->getActivatorsInScene()};
        api["actors"] = LObjectList{worldView->getActorsInScene()};
        api["containers"] = LObjectList{worldView->getContainersInScene()};
        api["doors"] = LObjectList{worldView->getDoorsInScene()};
        api["items"] = LObjectList{worldView->getItemsInScene()};

        api["NAVIGATOR_FLAGS"] = LuaUtil::makeStrictReadOnly(
            context.mLua->tableFromPairs<std::string_view, DetourNavigator::Flag>({
                {"Walk", DetourNavigator::Flag_walk},
                {"Swim", DetourNavigator::Flag_swim},
                {"OpenDoor", DetourNavigator::Flag_openDoor},
                {"UsePathgrid", DetourNavigator::Flag_usePathgrid},
            }));

        api["COLLISION_SHAPE_TYPE"] = LuaUtil::makeStrictReadOnly(
            context.mLua->tableFromPairs<std::string_view, DetourNavigator::CollisionShapeType>({
                {"Aabb", DetourNavigator::CollisionShapeType::Aabb},
                {"RotatingBox", DetourNavigator::CollisionShapeType::RotatingBox},
                {"Cylinder", DetourNavigator::CollisionShapeType::Cylinder},
            }));

        api["FIND_PATH_STATUS"] = LuaUtil::makeStrictReadOnly(
            context.mLua->tableFromPairs<std::string_view, DetourNavigator::Status>({
                {"Success", DetourNavigator::Status::Success},
                {"PartialPath", DetourNavigator::Status::PartialPath},
                {"NavMeshNotFound", DetourNavigator::Status::NavMeshNotFound},
                {"StartPolygonNotFound", DetourNavigator::Status::StartPolygonNotFound},
                {"EndPolygonNotFound", DetourNavigator::Status::EndPolygonNotFound},
                {"MoveAlongSurfaceFailed", DetourNavigator::Status::MoveAlongSurfaceFailed},
                {"FindPathOverPolygonsFailed", DetourNavigator::Status::FindPathOverPolygonsFailed},
                {"GetPolyHeightFailed", DetourNavigator::Status::GetPolyHeightFailed},
                {"InitNavMeshQueryFailed", DetourNavigator::Status::InitNavMeshQueryFailed},
            }));

        static const DetourNavigator::AgentBounds defaultAgentBounds {
            DetourNavigator::toCollisionShapeType(Settings::Manager::getInt("actor collision shape type", "Game")),
            Settings::Manager::getVector3("default actor pathfind half extents", "Game"),
        };
        static const float defaultStepSize = 2 * std::max(defaultAgentBounds.mHalfExtents.x(), defaultAgentBounds.mHalfExtents.y());
        static constexpr DetourNavigator::Flags defaultIncludeFlags = DetourNavigator::Flag_walk
                | DetourNavigator::Flag_swim
                | DetourNavigator::Flag_openDoor
                | DetourNavigator::Flag_usePathgrid;

        api["findPath"] = [] (const osg::Vec3f& source, const osg::Vec3f& destination,
            const sol::optional<sol::table>& options)
        {
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            float stepSize = defaultStepSize;
            DetourNavigator::Flags includeFlags = defaultIncludeFlags;
            DetourNavigator::AreaCosts areaCosts {};
            float destinationTolerance = 1;

            if (options.has_value())
            {
                if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                {
                    if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                        agentBounds.mShapeType = *v;
                    if (const auto& v = t->get<sol::optional<osg::Vec3f>>("halfExtents"))
                    {
                        agentBounds.mHalfExtents = *v;
                        stepSize = 2 * std::max(v->x(), v->y());
                    }
                }
                if (const auto& v = options->get<sol::optional<float>>("stepSize"))
                    stepSize = *v;
                if (const auto& v = options->get<sol::optional<DetourNavigator::Flags>>("includeFlags"))
                    includeFlags = *v;
                if (const auto& t = options->get<sol::optional<sol::table>>("areaCosts"))
                {
                    if (const auto& v = t->get<sol::optional<float>>("water"))
                        areaCosts.mWater = *v;
                    if (const auto& v = t->get<sol::optional<float>>("door"))
                        areaCosts.mDoor = *v;
                    if (const auto& v = t->get<sol::optional<float>>("pathgrid"))
                        areaCosts.mPathgrid = *v;
                    if (const auto& v = t->get<sol::optional<float>>("ground"))
                        areaCosts.mGround = *v;
                }
                if (const auto& v = options->get<sol::optional<float>>("destinationTolerance"))
                    destinationTolerance = *v;
            }

            std::vector<osg::Vec3f> result;

            const DetourNavigator::Status status = DetourNavigator::findPath(
                *MWBase::Environment::get().getWorld()->getNavigator(), agentBounds, stepSize, source,
                destination, includeFlags, areaCosts, destinationTolerance, std::back_inserter(result));

            return std::make_tuple(status, std::move(result));
        };

        api["findRandomPointAroundCircle"] = [] (const osg::Vec3f& position, float maxRadius,
            const sol::optional<sol::table>& options)
        {
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            DetourNavigator::Flags includeFlags = defaultIncludeFlags;

            if (options.has_value())
            {
                if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                {
                    if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                        agentBounds.mShapeType = *v;
                    if (const auto& v = t->get<sol::optional<osg::Vec3f>>("halfExtents"))
                        agentBounds.mHalfExtents = *v;
                }
                if (const auto& v = options->get<sol::optional<DetourNavigator::Flags>>("includeFlags"))
                    includeFlags = *v;
            }

            constexpr auto getRandom = []
            {
                return Misc::Rng::rollProbability(MWBase::Environment::get().getWorld()->getPrng());
            };

            return DetourNavigator::findRandomPointAroundCircle(*MWBase::Environment::get().getWorld()->getNavigator(),
                agentBounds, position, maxRadius, includeFlags, getRandom);
        };

        api["castNavigationRay"] = [] (const osg::Vec3f& from, const osg::Vec3f& to,
            const sol::optional<sol::table>& options)
        {
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            DetourNavigator::Flags includeFlags = defaultIncludeFlags;

            if (options.has_value())
            {
                if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                {
                    if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                        agentBounds.mShapeType = *v;
                    if (const auto& v = t->get<sol::optional<osg::Vec3f>>("halfExtents"))
                        agentBounds.mHalfExtents = *v;
                }
                if (const auto& v = options->get<sol::optional<DetourNavigator::Flags>>("includeFlags"))
                    includeFlags = *v;
            }

            return DetourNavigator::raycast(*MWBase::Environment::get().getWorld()->getNavigator(),
                agentBounds, from, to, includeFlags);
        };

        return LuaUtil::makeReadOnly(api);
    }
}
