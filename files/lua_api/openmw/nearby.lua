---
-- `openmw.nearby` provides read-only access to the nearest area of the game world.
-- Can be used only from local scripts.
-- @module nearby
-- @usage local nearby = require('openmw.nearby')



---
-- List of nearby activators.
-- @field [parent=#nearby] openmw.core#ObjectList activators

---
-- List of nearby actors.
-- @field [parent=#nearby] openmw.core#ObjectList actors

---
-- List of nearby containers.
-- @field [parent=#nearby] openmw.core#ObjectList containers

---
-- List of nearby doors.
-- @field [parent=#nearby] openmw.core#ObjectList doors

---
-- Everything nearby that is derived from @{openmw.types#Item}.
-- @field [parent=#nearby] openmw.core#ObjectList items

---
-- List of nearby players. Currently (since multiplayer is not yet implemented) always has one element.
-- @field [parent=#nearby] openmw.core#ObjectList players

---
-- Return an object by RefNum/FormId.
-- Note: the function always returns @{openmw.core#GameObject} and doesn't validate that
-- the object exists in the game world. If it doesn't exist or not yet loaded to memory),
-- then `obj:isValid()` will be `false`.
-- @function [parent=#nearby] getObjectByFormId
-- @param #string formId String returned by `core.getFormId`
-- @return openmw.core#GameObject
-- @usage local obj = nearby.getObjectByFormId(core.getFormId('Morrowind.esm', 128964))

---
-- @type COLLISION_TYPE
-- @field [parent=#COLLISION_TYPE] #number World
-- @field [parent=#COLLISION_TYPE] #number Door
-- @field [parent=#COLLISION_TYPE] #number Actor
-- @field [parent=#COLLISION_TYPE] #number HeightMap
-- @field [parent=#COLLISION_TYPE] #number Projectile
-- @field [parent=#COLLISION_TYPE] #number Water
-- @field [parent=#COLLISION_TYPE] #number Default Used by default: World+Door+Actor+HeightMap
-- @field [parent=#COLLISION_TYPE] #number AnyPhysical World+Door+Actor+HeightMap+Projectile+Water
-- @field [parent=#COLLISION_TYPE] #number Camera Objects that should collide only with camera
-- @field [parent=#COLLISION_TYPE] #number VisualOnly Objects that were not intended to be part of the physics world

---
-- Collision types that are used in `castRay`.
-- Several types can be combined with @{openmw_util#util.bitOr}.
-- @field [parent=#nearby] #COLLISION_TYPE COLLISION_TYPE

---
-- Result of raycasting
-- @type RayCastingResult
-- @field [parent=#RayCastingResult] #boolean hit Is there a collision? (true/false)
-- @field [parent=#RayCastingResult] openmw.util#Vector3 hitPos Position of the collision point (nil if no collision)
-- @field [parent=#RayCastingResult] openmw.util#Vector3 hitNormal Normal to the surface in the collision point (nil if no collision)
-- @field [parent=#RayCastingResult] openmw.core#GameObject hitObject The object the ray has collided with (can be nil)

---
-- A table of parameters for @{#nearby.castRay}
-- @type CastRayOptions
-- @field #any ignore An @{openmw.core#GameObject} or @{openmw.core#ObjectList} to ignore (specify here the source of the ray, or other objects which should not collide)
-- @field #number collisionType Object types to work with (see @{openmw.nearby#COLLISION_TYPE})
-- @field #number radius The radius of the ray (zero by default). If not zero then castRay actually casts a sphere with given radius.
--  NOTE: currently `ignore` is not supported if `radius>0`.

---
-- Cast ray from one point to another and return the first collision.
-- @function [parent=#nearby] castRay
-- @param openmw.util#Vector3 from Start point of the ray.
-- @param openmw.util#Vector3 to End point of the ray.
-- @param #CastRayOptions options An optional table with additional optional arguments
-- @return #RayCastingResult
-- @usage if nearby.castRay(pointA, pointB).hit then print('obstacle between A and B') end
-- @usage local res = nearby.castRay(self.position, enemy.position, {ignore=self})
-- if res.hitObject and res.hitObject ~= enemy then obstacle = res.hitObject end
-- @usage local res = nearby.castRay(self.position, targetPos, {
--     collisionType=nearby.COLLISION_TYPE.HeightMap + nearby.COLLISION_TYPE.Water,
--     radius = 10,
-- })

---
-- A table of parameters for @{#nearby.castRenderingRay} and @{#nearby.asyncCastRenderingRay}
-- @type CastRenderingRayOptions
-- @field #any ignore A @{openmw.core#GameObject} or @{openmw.core#ObjectList} to ignore while doing the ray cast

---
-- Cast ray from one point to another and find the first visual intersection with anything in the scene.
-- As opposite to `castRay` can find an intersection with an object without collisions.
-- In order to avoid threading issues can be used only in player scripts only in `onFrame` or
-- in engine handlers for user input. In other cases use `asyncCastRenderingRay` instead.
-- @function [parent=#nearby] castRenderingRay
-- @param openmw.util#Vector3 from Start point of the ray.
-- @param openmw.util#Vector3 to End point of the ray.
-- @param #CastRenderingRayOptions options An optional table with additional optional arguments
-- @return #RayCastingResult

---
-- Asynchronously cast ray from one point to another and find the first visual intersection with anything in the scene.
-- @function [parent=#nearby] asyncCastRenderingRay
-- @param openmw.async#Callback callback The callback to pass the result to (should accept a single argument @{openmw.nearby#RayCastingResult}).
-- @param openmw.util#Vector3 from Start point of the ray.
-- @param openmw.util#Vector3 to End point of the ray.
-- @param #CastRenderingRayOptions

---
-- @type NAVIGATOR_FLAGS
-- @field [parent=#NAVIGATOR_FLAGS] #number Walk Allow agent to walk on the ground area.
-- @field [parent=#NAVIGATOR_FLAGS] #number Swim Allow agent to swim on the water surface.
-- @field [parent=#NAVIGATOR_FLAGS] #number OpenDoor Allow agent to open doors on the way.
-- @field [parent=#NAVIGATOR_FLAGS] #number UsePathgrid Allow agent to use predefined pathgrid imported from ESM files.

---
-- @type COLLISION_SHAPE_TYPE
-- @field [parent=#COLLISION_SHAPE_TYPE] #number Aabb Axis-Aligned Bounding Box is used for NPC and symmetric
-- Creatures.
-- @field [parent=#COLLISION_SHAPE_TYPE] #number RotatingBox is used for Creatures with big difference in width and
-- height.
-- @field [parent=#COLLISION_SHAPE_TYPE] #number Cylinder is used for NPC and symmetric Creatures.

---
-- @type FIND_PATH_STATUS
-- @field [parent=#FIND_PATH_STATUS] #number Success Path is found.
-- @field [parent=#FIND_PATH_STATUS] #number PartialPath Last path point is not a destination but a nearest position
-- among found;
-- @field [parent=#FIND_PATH_STATUS] #number NavMeshNotFound Provided `agentBounds` don't have corresponding navigation
-- mesh. For interior cells it means an agent with such `agentBounds` is present on the scene. For exterior cells only
-- default `agentBounds` is supported;
-- @field [parent=#FIND_PATH_STATUS] #number StartPolygonNotFound `source` position is too far from available
-- navigation mesh. The status may appear when navigation mesh is not fully generated or position is outside of covered
-- area;
-- @field [parent=#FIND_PATH_STATUS] #number EndPolygonNotFound `destination` position is too far from available
-- navigation mesh. The status may appear when navigation mesh is not fully generated or position is outside of covered
-- area;
-- @field [parent=#FIND_PATH_STATUS] #number TargetPolygonNotFound adjusted `destination` position is too far from
-- available navigation mesh. The status may appear when navigation mesh is not fully generated or position is outside
-- of covered area;
-- @field [parent=#FIND_PATH_STATUS] #number MoveAlongSurfaceFailed Found path couldn't be smoothed due to imperfect
-- algorithm implementation or bad navigation mesh data;
-- @field [parent=#FIND_PATH_STATUS] #number FindPathOverPolygonsFailed Path over navigation mesh from `source` to
-- `destination` does not exist or navigation mesh is not fully generated to provide the path;
-- @field [parent=#FIND_PATH_STATUS] #number InitNavMeshQueryFailed Couldn't initialize required data due to bad input
-- or bad navigation mesh data.
-- @field [parent=#FIND_PATH_STATUS] #number FindStraightPathFailed Couldn't map path over polygons into world
-- coordinates.

---
-- A table of parameters identifying navmesh
-- @type AgentBounds
-- @field [parent=#AgentBounds] #COLLISION_SHAPE_TYPE shapeType.
-- @field [parent=#AgentBounds] openmw.util#Vector3 halfExtents.

---
-- A table of parameters to specify relative path cost per each area type
-- @type AreaCosts
-- @field [parent=#AreaCosts] #number ground Value >= 0, used in combination with @{#NAVIGATOR_FLAGS.Walk} (default: 1).
-- @field [parent=#AreaCosts] #number water Value >= 0, used in combination with @{#NAVIGATOR_FLAGS.Swim} (default: 1).
-- @field [parent=#AreaCosts] #number door Value >= 0, used in combination with @{#NAVIGATOR_FLAGS.OpenDoor}
-- (default: 2).
-- @field [parent=#AreaCosts] #number pathgrid Value >= 0, used in combination with @{#NAVIGATOR_FLAGS.UsePathgrid}
-- (default: 1).

---
-- A table of parameters for @{#nearby.findPath}
-- @type FindPathOptions
-- @field [parent=#FindPathOptions] #AgentBounds agentBounds identifies which navmesh to use.
-- @field [parent=#FindPathOptions] #number includeFlags allowed areas for agent to move, a sum of @{#NAVIGATOR_FLAGS}
-- values (default: @{#NAVIGATOR_FLAGS.Walk} + @{#NAVIGATOR_FLAGS.Swim} + @{#NAVIGATOR_FLAGS.OpenDoor}
-- + @{#NAVIGATOR_FLAGS.UsePathgrid}).
-- @field [parent=#FindPathOptions] #AreaCosts areaCosts a table defining relative cost for each type of area.
-- @field [parent=#FindPathOptions] #number destinationTolerance a floating point number representing maximum allowed
-- distance between destination and a nearest point on the navigation mesh in addition to agent size (default: 1).

---
-- A table of parameters for @{#nearby.findRandomPointAroundCircle} and @{#nearby.castNavigationRay}
-- @type NavMeshOptions
-- @field [parent=#NavMeshOptions] #AgentBounds agentBounds Identifies which navmesh to use.
-- @field [parent=#NavMeshOptions] #number includeFlags Allowed areas for agent to move, a sum of @{#NAVIGATOR_FLAGS}
-- values (default: @{#NAVIGATOR_FLAGS.Walk} + @{#NAVIGATOR_FLAGS.Swim} + @{#NAVIGATOR_FLAGS.OpenDoor}
-- + @{#NAVIGATOR_FLAGS.UsePathgrid}).

---
-- A table of parameters for @{#nearby.findNearestNavMeshPosition}
-- @type FindNearestNavMeshPositionOptions
-- @field [parent=#NavMeshOptions] #AgentBounds agentBounds Identifies which navmesh to use.
-- @field [parent=#NavMeshOptions] #number includeFlags Allowed areas for agent to move, a sum of @{#NAVIGATOR_FLAGS}
-- values (default: @{#NAVIGATOR_FLAGS.Walk} + @{#NAVIGATOR_FLAGS.Swim} + @{#NAVIGATOR_FLAGS.OpenDoor}
-- + @{#NAVIGATOR_FLAGS.UsePathgrid}).
-- @field [parent=#NavMeshOptions] openmw.util#Vector3 searchAreaHalfExtents Defines AABB like area half extents around
-- given position (default: (1 + 2 * CellGridRadius) * CellSize * (1, 1, 1) where CellGridRadius and depends on cell
-- type to cover the whole active grid).

---
-- Find path over navigation mesh from source to destination with given options. Result is unstable since navigation
-- mesh generation is asynchronous.
-- @function [parent=#nearby] findPath
-- @param openmw.util#Vector3 source Initial path position.
-- @param openmw.util#Vector3 destination Final path position.
-- @param #FindPathOptions options An optional table with additional optional arguments.
-- @return #FIND_PATH_STATUS
-- @return #list<openmw.util#Vector3>
-- @usage local status, path = nearby.findPath(source, destination)
-- @usage local status, path = nearby.findPath(source, destination, {
--     includeFlags = nearby.NAVIGATOR_FLAGS.Walk + nearby.NAVIGATOR_FLAGS.OpenDoor,
--     areaCosts = {
--         door = 1.5,
--     },
-- })
-- @usage local status, path = nearby.findPath(source, destination, {
--     agentBounds = Actor.getPathfindingAgentBounds(self),
-- })

---
-- Returns random location on navigation mesh within the reach of specified location.
-- The location is not exactly constrained by the circle, but it limits the area.
-- @function [parent=#nearby] findRandomPointAroundCircle
-- @param openmw.util#Vector3 position Center of the search circle.
-- @param #number maxRadius Approximate maximum search distance.
-- @param #NavMeshOptions options An optional table with additional optional arguments.
-- @return openmw.util#Vector3, #nil
-- @usage local position = nearby.findRandomPointAroundCircle(position, maxRadius)
-- @usage local position = nearby.findRandomPointAroundCircle(position, maxRadius, {
--     includeFlags = nearby.NAVIGATOR_FLAGS.Walk,
-- })
-- @usage local position = nearby.findRandomPointAroundCircle(position, maxRadius, {
--     agentBounds = Actor.getPathfindingAgentBounds(self),
-- })

---
-- Finds a nearest to the ray target position starting from the initial position with resulting curve drawn on the
-- navigation mesh surface.
-- @function [parent=#nearby] castNavigationRay
-- @param openmw.util#Vector3 from Initial ray position.
-- @param openmw.util#Vector3 to Target ray position.
-- @param #NavMeshOptions options An optional table with additional optional arguments.
-- @return openmw.util#Vector3, #nil
-- @usage local position = nearby.castNavigationRay(from, to)
-- @usage local position = nearby.castNavigationRay(from, to, {
--     includeFlags = nearby.NAVIGATOR_FLAGS.Swim,
-- })
-- @usage local position = nearby.castNavigationRay(from, to, {
--     agentBounds = Actor.getPathfindingAgentBounds(self),
-- })

---
-- Finds a nearest position on navigation mesh to the given position within given search area.
-- @function [parent=#nearby] findNearestNavMeshPosition
-- @param openmw.util#Vector3 position Search area center.
-- @param #FindNearestNavMeshPositionOptions options An optional table with additional optional arguments.
-- @return openmw.util#Vector3, #nil
-- @usage local navMeshPosition = nearby.findNearestNavMeshPosition(position)
-- @usage local navMeshPosition = nearby.findNearestNavMeshPosition(position, {
--     includeFlags = nearby.NAVIGATOR_FLAGS.Swim,
-- })
-- @usage local navMeshPosition = nearby.findNearestNavMeshPosition(position, {
--     agentBounds = Actor.getPathfindingAgentBounds(self),
-- })
-- @usage local navMeshPosition = nearby.findNearestNavMeshPosition(position, {
--     searchAreaHalfExtents = util.vector3(1000, 1000, 1000),
--     includeFlags = nearby.NAVIGATOR_FLAGS.Walk,
-- })

return nil
