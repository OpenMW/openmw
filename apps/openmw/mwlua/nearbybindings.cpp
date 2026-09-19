#include "nearbybindings.hpp"

#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/navigatorutils.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/convert.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwphysics/raycasting.hpp"
#include "../mwworld/cell.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/scene.hpp"

#include "context.hpp"
#include "luamanagerimp.hpp"
#include "objectlists.hpp"

#include <optional>
#include <vector>

namespace
{
    template <class T = MWWorld::Ptr>
    std::vector<T> parseIgnoreList(const sol::table& options)
    {
        std::vector<T> ignore;

        if (const auto& ignoreObj = options.get<sol::optional<sol::object>>("ignore"))
        {
            if (ignoreObj->is<MWLua::LObject>())
                ignore.push_back(ignoreObj->as<MWLua::LObject>().ptr());
            else if (ignoreObj->is<MWLua::LObjectList>())
            {
                for (const MWLua::ObjectId& id : *ignoreObj->as<MWLua::LObjectList>().mIds)
                {
                    ignore.push_back(MWLua::LObject(id).ptr());
                }
            }
            else
            {
                // ignoreObj->as throws if the type doesn't match, but an unchecked value.as crashes...
                ignoreObj->as<sol::lua_table>().for_each([&](sol::object _, sol::object value) {
                    if (value.is<MWLua::LObject>())
                        ignore.push_back(value.as<MWLua::LObject>().ptr());
                    else
                        throw std::runtime_error("Table value is not a GameObject");
                });
            }
        }

        return ignore;
    }

    std::optional<glm::vec3> toGlm(const std::optional<osg::Vec3f>& value)
    {
        if (!value.has_value())
            return std::nullopt;
        return Misc::Convert::toGlm(*value);
    }
}

namespace sol
{
    template <>
    struct is_automagical<MWPhysics::RayCastingResult> : std::false_type
    {
    };
}

namespace MWLua
{
    sol::table initNearbyPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);
        ObjectLists* objectLists = context.mObjectLists;

        sol::usertype<MWPhysics::RayCastingResult> rayResult
            = lua.new_usertype<MWPhysics::RayCastingResult>("RayCastingResult");
        rayResult["hit"] = sol::readonly_property([](const MWPhysics::RayCastingResult& r) { return r.mHit; });
        rayResult["hitPos"]
            = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<glm::vec3> {
                  if (r.mHit)
                      return Misc::Convert::toGlm(r.mHitPos);
                  else
                      return sol::nullopt;
              });
        rayResult["hitNormal"]
            = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<glm::vec3> {
                  if (r.mHit)
                      return Misc::Convert::toGlm(r.mHitNormal);
                  else
                      return sol::nullopt;
              });
        rayResult["hitObject"]
            = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<LObject> {
                  if (r.mHitObject.isEmpty())
                      return sol::nullopt;
                  else
                      return LObject(getId(r.mHitObject));
              });

        api["COLLISION_TYPE"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, MWPhysics::CollisionType>(lua,
                {
                    { "World", MWPhysics::CollisionType_World },
                    { "Door", MWPhysics::CollisionType_Door },
                    { "Actor", MWPhysics::CollisionType_Actor },
                    { "HeightMap", MWPhysics::CollisionType_HeightMap },
                    { "Projectile", MWPhysics::CollisionType_Projectile },
                    { "Water", MWPhysics::CollisionType_Water },
                    { "Default", MWPhysics::CollisionType_Default },
                    { "AnyPhysical", MWPhysics::CollisionType_AnyPhysical },
                    { "Camera", MWPhysics::CollisionType_CameraOnly },
                    { "VisualOnly", MWPhysics::CollisionType_VisualOnly },
                }));

        api["castRay"] = [](const glm::vec3& fromIn, const glm::vec3& toIn, sol::optional<sol::table> options) {
            const osg::Vec3f from = Misc::Convert::toOsg(fromIn);
            const osg::Vec3f to = Misc::Convert::toOsg(toIn);
            std::vector<MWWorld::ConstPtr> ignore;
            int collisionType = MWPhysics::CollisionType_Default;
            float radius = 0;
            if (options)
            {
                ignore = parseIgnoreList<MWWorld::ConstPtr>(*options);
                collisionType = options->get<sol::optional<int>>("collisionType").value_or(collisionType);
                radius = options->get<sol::optional<float>>("radius").value_or(0);
            }
            const MWPhysics::RayCastingInterface* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();
            if (radius <= 0)
            {
                return rayCasting->castRay(from, to, ignore, {}, collisionType);
            }
            else
            {
                for (const auto& ptr : ignore)
                {
                    if (!ptr.isEmpty())
                        throw std::logic_error("Currently castRay doesn't support `ignore` when radius > 0");
                }
                return rayCasting->castSphere(from, to, radius, collisionType);
            }
        };
        // TODO: async raycasting
        /*api["asyncCastRay"] = [luaManager = context.mLuaManager](
            const Callback& luaCallback, const osg::Vec3f& from, const osg::Vec3f& to, sol::optional<sol::table>
        options)
        {
            std::function<void(MWPhysics::RayCastingResult)> callback =
                luaManager->wrapLuaCallback<MWPhysics::RayCastingResult>(luaCallback);
            MWPhysics::RayCastingInterface* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();

            // Handle options the same way as in `castRay`.

            // NOTE: `callback` is not thread safe. If MWPhysics works in separate thread, it must put results to a
        queue
            //       and use this callback from the main thread at the beginning of the next frame processing.
            rayCasting->asyncCastRay(callback, from, to, ignore, std::vector<MWWorld::Ptr>(), collisionType);
        };*/
        api["castRenderingRay"] = [manager = context.mLuaManager](const glm::vec3& fromIn, const glm::vec3& toIn,
                                      const sol::optional<sol::table>& options) {
            const osg::Vec3f from = Misc::Convert::toOsg(fromIn);
            const osg::Vec3f to = Misc::Convert::toOsg(toIn);
            if (!manager->isProcessingInputEvents())
            {
                throw std::logic_error(
                    "castRenderingRay can be used only in player scripts during processing of input events; "
                    "use asyncCastRenderingRay instead.");
            }

            std::vector<MWWorld::Ptr> ignore;
            if (options.has_value())
            {
                ignore = parseIgnoreList(*options);
            }

            MWPhysics::RayCastingResult res;
            MWBase::Environment::get().getWorld()->castRenderingRay(res, from, to, false, false, false, ignore);
            return res;
        };
        api["asyncCastRenderingRay"] = [context](const sol::table& callback, const glm::vec3& fromIn,
                                           const glm::vec3& toIn, const sol::optional<sol::table>& options) {
            const osg::Vec3f from = Misc::Convert::toOsg(fromIn);
            const osg::Vec3f to = Misc::Convert::toOsg(toIn);
            std::vector<MWWorld::Ptr> ignore;
            if (options.has_value())
            {
                ignore = parseIgnoreList(*options);
            }

            context.mLuaManager->addAction(
                [context, ignore = std::move(ignore), callback = LuaUtil::Callback::fromLua(callback), from, to] {
                    MWPhysics::RayCastingResult res;
                    MWBase::Environment::get().getWorld()->castRenderingRay(res, from, to, false, false, false, ignore);
                    context.mLuaManager->queueCallback(
                        callback, sol::main_object(context.mLua->unsafeState(), sol::in_place, res));
                });
        };

        api["getObjectByFormId"] = [](std::string_view formIdStr) -> LObject {
            ESM::RefId refId = ESM::RefId::deserializeText(formIdStr);
            if (!refId.is<ESM::FormId>())
                throw std::runtime_error("FormId expected, got " + std::string(formIdStr) + "; use core.getFormId");
            return LObject(*refId.getIf<ESM::FormId>());
        };

        api["activators"] = LObjectList{ objectLists->getActivatorsInScene() };
        api["actors"] = LObjectList{ objectLists->getActorsInScene() };
        api["containers"] = LObjectList{ objectLists->getContainersInScene() };
        api["doors"] = LObjectList{ objectLists->getDoorsInScene() };
        api["items"] = LObjectList{ objectLists->getItemsInScene() };
        api["players"] = LObjectList{ objectLists->getPlayers() };

        api["NAVIGATOR_FLAGS"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, DetourNavigator::Flag>(lua,
                {
                    { "Walk", DetourNavigator::Flag_walk },
                    { "Swim", DetourNavigator::Flag_swim },
                    { "OpenDoor", DetourNavigator::Flag_openDoor },
                    { "UsePathgrid", DetourNavigator::Flag_usePathgrid },
                }));

        api["COLLISION_SHAPE_TYPE"] = LuaUtil::makeStrictReadOnly(
            LuaUtil::tableFromPairs<std::string_view, DetourNavigator::CollisionShapeType>(lua,
                {
                    { "Aabb", DetourNavigator::CollisionShapeType::Aabb },
                    { "RotatingBox", DetourNavigator::CollisionShapeType::RotatingBox },
                    { "Cylinder", DetourNavigator::CollisionShapeType::Cylinder },
                }));

        api["FIND_PATH_STATUS"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, DetourNavigator::Status>(lua,
                {
                    { "Success", DetourNavigator::Status::Success },
                    { "PartialPath", DetourNavigator::Status::PartialPath },
                    { "NavMeshNotFound", DetourNavigator::Status::NavMeshNotFound },
                    { "StartPolygonNotFound", DetourNavigator::Status::StartPolygonNotFound },
                    { "EndPolygonNotFound", DetourNavigator::Status::EndPolygonNotFound },
                    { "TargetPolygonNotFound", DetourNavigator::Status::TargetPolygonNotFound },
                    { "MoveAlongSurfaceFailed", DetourNavigator::Status::MoveAlongSurfaceFailed },
                    { "FindPathOverPolygonsFailed", DetourNavigator::Status::FindPathOverPolygonsFailed },
                    { "InitNavMeshQueryFailed", DetourNavigator::Status::InitNavMeshQueryFailed },
                    { "FindStraightPathFailed", DetourNavigator::Status::FindStraightPathFailed },
                }));

        static const DetourNavigator::AgentBounds defaultAgentBounds{
            Settings::game().mActorCollisionShapeType,
            Settings::game().mDefaultActorPathfindHalfExtents,
        };
        static constexpr DetourNavigator::Flags defaultIncludeFlags = DetourNavigator::Flag_walk
            | DetourNavigator::Flag_swim | DetourNavigator::Flag_openDoor | DetourNavigator::Flag_usePathgrid;

        api["findPath"] = [lua](const glm::vec3& sourceIn, const glm::vec3& destinationIn,
                              const sol::optional<sol::table>& options) {
            const osg::Vec3f source = Misc::Convert::toOsg(sourceIn);
            const osg::Vec3f destination = Misc::Convert::toOsg(destinationIn);
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            DetourNavigator::Flags includeFlags = defaultIncludeFlags;
            DetourNavigator::AreaCosts areaCosts{};
            float destinationTolerance = 1;
            std::vector<osg::Vec3f> checkpoints;

            if (options.has_value())
            {
                if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                {
                    if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                        agentBounds.mShapeType = *v;
                    if (const auto& v = t->get<sol::optional<glm::vec3>>("halfExtents"))
                        agentBounds.mHalfExtents = Misc::Convert::toOsg(*v);
                }
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
                if (const auto& t = options->get<sol::optional<sol::table>>("checkpoints"))
                {
                    for (const auto& [k, v] : *t)
                    {
                        const int index = k.as<int>();
                        const osg::Vec3f position = Misc::Convert::toOsg(v.as<glm::vec3>());
                        if (index != static_cast<int>(checkpoints.size() + 1))
                            throw std::runtime_error("checkpoints is not an array");
                        checkpoints.push_back(position);
                    }
                }
            }

            std::vector<osg::Vec3f> path;

            const DetourNavigator::Status status
                = DetourNavigator::findPath(*MWBase::Environment::get().getWorld()->getNavigator(), agentBounds, source,
                    destination, includeFlags, areaCosts, destinationTolerance, checkpoints, std::back_inserter(path));

            sol::table result(lua, sol::create);
            for (size_t i = 0; i < path.size(); ++i)
                result[i + 1] = Misc::Convert::toGlm(path[i]);
            return std::make_tuple(status, result);
        };

        api["findRandomPointAroundCircle"] = [](const glm::vec3& positionIn, float maxRadius,
                                                 const sol::optional<sol::table>& options) {
            const osg::Vec3f position = Misc::Convert::toOsg(positionIn);
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            DetourNavigator::Flags includeFlags = defaultIncludeFlags;

            if (options.has_value())
            {
                if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                {
                    if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                        agentBounds.mShapeType = *v;
                    if (const auto& v = t->get<sol::optional<glm::vec3>>("halfExtents"))
                        agentBounds.mHalfExtents = Misc::Convert::toOsg(*v);
                }
                if (const auto& v = options->get<sol::optional<DetourNavigator::Flags>>("includeFlags"))
                    includeFlags = *v;
            }

            constexpr auto getRandom
                = [] { return Misc::Rng::rollProbability(MWBase::Environment::get().getWorld()->getPrng()); };

            return toGlm(
                DetourNavigator::findRandomPointAroundCircle(*MWBase::Environment::get().getWorld()->getNavigator(),
                    agentBounds, position, maxRadius, includeFlags, getRandom));
        };

        api["castNavigationRay"]
            = [](const glm::vec3& fromIn, const glm::vec3& toIn, const sol::optional<sol::table>& options) {
                  const osg::Vec3f from = Misc::Convert::toOsg(fromIn);
                  const osg::Vec3f to = Misc::Convert::toOsg(toIn);
                  DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
                  DetourNavigator::Flags includeFlags = defaultIncludeFlags;

                  if (options.has_value())
                  {
                      if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                      {
                          if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                              agentBounds.mShapeType = *v;
                          if (const auto& v = t->get<sol::optional<glm::vec3>>("halfExtents"))
                              agentBounds.mHalfExtents = Misc::Convert::toOsg(*v);
                      }
                      if (const auto& v = options->get<sol::optional<DetourNavigator::Flags>>("includeFlags"))
                          includeFlags = *v;
                  }

                  return toGlm(DetourNavigator::raycast(
                      *MWBase::Environment::get().getWorld()->getNavigator(), agentBounds, from, to, includeFlags));
              };

        api["findNearestNavMeshPosition"] = [](const glm::vec3& positionIn, const sol::optional<sol::table>& options) {
            const osg::Vec3f position = Misc::Convert::toOsg(positionIn);
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            std::optional<osg::Vec3f> searchAreaHalfExtents;
            DetourNavigator::Flags includeFlags = defaultIncludeFlags;

            if (options.has_value())
            {
                if (const auto& t = options->get<sol::optional<sol::table>>("agentBounds"))
                {
                    if (const auto& v = t->get<sol::optional<DetourNavigator::CollisionShapeType>>("shapeType"))
                        agentBounds.mShapeType = *v;
                    if (const auto& v = t->get<sol::optional<glm::vec3>>("halfExtents"))
                        agentBounds.mHalfExtents = Misc::Convert::toOsg(*v);
                }
                if (const auto& v = options->get<sol::optional<glm::vec3>>("searchAreaHalfExtents"))
                    searchAreaHalfExtents = Misc::Convert::toOsg(*v);
                if (const auto& v = options->get<sol::optional<DetourNavigator::Flags>>("includeFlags"))
                    includeFlags = *v;
            }

            if (!searchAreaHalfExtents.has_value())
            {
                const bool isEsm4 = MWBase::Environment::get().getWorldScene()->getCurrentCell()->getCell()->isEsm4();
                const float halfExtents = static_cast<float>(isEsm4
                        ? (1 + 2 * Constants::ESM4CellGridRadius) * Constants::ESM4CellSizeInUnits
                        : (1 + 2 * Constants::CellGridRadius) * Constants::CellSizeInUnits);
                searchAreaHalfExtents = osg::Vec3f(halfExtents, halfExtents, halfExtents);
            }

            return toGlm(
                DetourNavigator::findNearestNavMeshPosition(*MWBase::Environment::get().getWorld()->getNavigator(),
                    agentBounds, position, *searchAreaHalfExtents, includeFlags));
        };

        return LuaUtil::makeReadOnly(api);
    }
}
