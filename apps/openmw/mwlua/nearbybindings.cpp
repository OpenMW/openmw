#include "nearbybindings.hpp"

#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/navigatorutils.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/constants.hpp>
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

#include <vector>

namespace
{
    template <class T = MWWorld::Ptr>
    std::vector<T> parseIgnoreList(const sol::table& options)
    {
        std::vector<T> ignore;

        if (const auto& ignoreObj = options.get<sol::optional<MWLua::LObject>>("ignore"))
        {
            ignore.push_back(ignoreObj->ptr());
        }
        else if (const auto& ignoreTable = options.get<sol::optional<sol::table>>("ignore"))
        {
            ignoreTable->for_each([&](const auto& _, const sol::object& value) {
                if (value.is<MWLua::LObject>())
                {
                    ignore.push_back(value.as<MWLua::LObject>().ptr());
                }
            });
        }

        return ignore;
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
            = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<osg::Vec3f> {
                  if (r.mHit)
                      return r.mHitPos;
                  else
                      return sol::nullopt;
              });
        rayResult["hitNormal"]
            = sol::readonly_property([](const MWPhysics::RayCastingResult& r) -> sol::optional<osg::Vec3f> {
                  if (r.mHit)
                      return r.mHitNormal;
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

        api["castRay"] = [](const osg::Vec3f& from, const osg::Vec3f& to, sol::optional<sol::table> options) {
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
        api["castRenderingRay"] = [manager = context.mLuaManager](const osg::Vec3f& from, const osg::Vec3f& to,
                                      const sol::optional<sol::table>& options) {
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
            MWBase::Environment::get().getWorld()->castRenderingRay(res, from, to, false, false, ignore);
            return res;
        };
        api["asyncCastRenderingRay"] = [context](const sol::table& callback, const osg::Vec3f& from,
                                           const osg::Vec3f& to, const sol::optional<sol::table>& options) {
            std::vector<MWWorld::Ptr> ignore;
            if (options.has_value())
            {
                ignore = parseIgnoreList(*options);
            }

            context.mLuaManager->addAction(
                [context, ignore = std::move(ignore), callback = LuaUtil::Callback::fromLua(callback), from, to] {
                    MWPhysics::RayCastingResult res;
                    MWBase::Environment::get().getWorld()->castRenderingRay(res, from, to, false, false, ignore);
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

        api["findPath"]
            = [lua](const osg::Vec3f& source, const osg::Vec3f& destination, const sol::optional<sol::table>& options) {
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
                          if (const auto& v = t->get<sol::optional<osg::Vec3f>>("halfExtents"))
                              agentBounds.mHalfExtents = *v;
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
                              const osg::Vec3f position = v.as<osg::Vec3f>();
                              if (index != static_cast<int>(checkpoints.size() + 1))
                                  throw std::runtime_error("checkpoints is not an array");
                              checkpoints.push_back(position);
                          }
                      }
                  }

                  std::vector<osg::Vec3f> path;

                  const DetourNavigator::Status status = DetourNavigator::findPath(
                      *MWBase::Environment::get().getWorld()->getNavigator(), agentBounds, source, destination,
                      includeFlags, areaCosts, destinationTolerance, checkpoints, std::back_inserter(path));

                  sol::table result(lua, sol::create);
                  LuaUtil::copyVectorToTable(path, result);
                  return std::make_tuple(status, result);
              };

        api["findRandomPointAroundCircle"] = [](const osg::Vec3f& position, float maxRadius,
                                                 const sol::optional<sol::table>& options) {
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

            constexpr auto getRandom
                = [] { return Misc::Rng::rollProbability(MWBase::Environment::get().getWorld()->getPrng()); };

            return DetourNavigator::findRandomPointAroundCircle(*MWBase::Environment::get().getWorld()->getNavigator(),
                agentBounds, position, maxRadius, includeFlags, getRandom);
        };

        api["castNavigationRay"]
            = [](const osg::Vec3f& from, const osg::Vec3f& to, const sol::optional<sol::table>& options) {
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

                  return DetourNavigator::raycast(
                      *MWBase::Environment::get().getWorld()->getNavigator(), agentBounds, from, to, includeFlags);
              };

        api["findNearestNavMeshPosition"] = [](const osg::Vec3f& position, const sol::optional<sol::table>& options) {
            DetourNavigator::AgentBounds agentBounds = defaultAgentBounds;
            std::optional<osg::Vec3f> searchAreaHalfExtents;
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
                if (const auto& v = options->get<sol::optional<osg::Vec3f>>("searchAreaHalfExtents"))
                    searchAreaHalfExtents = *v;
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

            return DetourNavigator::findNearestNavMeshPosition(*MWBase::Environment::get().getWorld()->getNavigator(),
                agentBounds, position, *searchAreaHalfExtents, includeFlags);
        };

        return LuaUtil::makeReadOnly(api);
    }
}
