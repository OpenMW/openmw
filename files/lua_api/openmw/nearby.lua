-------------------------------------------------------------------------------
-- `openmw.nearby` provides read-only access to the nearest area of the game world.
-- Can be used only from local scripts.
-- @module nearby
-- @usage local nearby = require('openmw.nearby')



-------------------------------------------------------------------------------
-- List of nearby activators.
-- @field [parent=#nearby] openmw.core#ObjectList activators

-------------------------------------------------------------------------------
-- List of nearby actors.
-- @field [parent=#nearby] openmw.core#ObjectList actors

-------------------------------------------------------------------------------
-- List of nearby containers.
-- @field [parent=#nearby] openmw.core#ObjectList containers

-------------------------------------------------------------------------------
-- List of nearby doors.
-- @field [parent=#nearby] openmw.core#ObjectList doors

-------------------------------------------------------------------------------
-- Everything that can be picked up in the nearby.
-- @field [parent=#nearby] openmw.core#ObjectList items

-------------------------------------------------------------------------------
-- Evaluates a Query.
-- @function [parent=#nearby] selectObjects
-- @param openmw.query#Query query
-- @return openmw.core#ObjectList

-------------------------------------------------------------------------------
-- @type COLLISION_TYPE
-- @field [parent=#COLLISION_TYPE] #number World
-- @field [parent=#COLLISION_TYPE] #number Door
-- @field [parent=#COLLISION_TYPE] #number Actor
-- @field [parent=#COLLISION_TYPE] #number HeightMap
-- @field [parent=#COLLISION_TYPE] #number Projectile
-- @field [parent=#COLLISION_TYPE] #number Water
-- @field [parent=#COLLISION_TYPE] #number Default Used by deafult: World+Door+Actor+HeightMap

-------------------------------------------------------------------------------
-- Collision types that are used in `castRay`.
-- Several types can be combined with '+'.
-- @field [parent=#nearby] #COLLISION_TYPE COLLISION_TYPE

-------------------------------------------------------------------------------
-- Result of raycasing
-- @type RayCastingResult
-- @field [parent=#RayCastingResult] #boolean hit Is there a collision? (true/false)
-- @field [parent=#RayCastingResult] openmw.util#Vector3 hitPos Position of the collision point (nil if no collision)
-- @field [parent=#RayCastingResult] openmw.util#Vector3 hitNormal Normal to the surface in the collision point (nil if no collision)
-- @field [parent=#RayCastingResult] openmw.core#GameObject hitObject The object the ray has collided with (can be nil)

-------------------------------------------------------------------------------
-- Cast ray from one point to another and return the first collision.
-- @function [parent=#nearby] castRay
-- @param openmw.util#Vector3 from Start point of the ray.
-- @param openmw.util#Vector3 to End point of the ray.
-- @param #table options An optional table with additional optional arguments. Can contain:  
-- `ignore` - an object to ignore (specify here the source of the ray);  
-- `collisionType` - object types to work with (see @{openmw.nearby#COLLISION_TYPE}), several types can be combined with '+';  
-- `radius` - the radius of the ray (zero by default). If not zero then castRay actually casts a sphere with given radius.  
-- NOTE: currently `ignore` is not supported if `radius>0`.
-- @return #RayCastingResult
-- @usage if nearby.castRay(pointA, pointB).hit then print('obstacle between A and B') end
-- @usage local res = nearby.castRay(self.position, enemy.position, {ignore=self})
-- if res.hitObject and res.hitObject ~= enemy then obstacle = res.hitObject end
-- @usage local res = nearby.castRay(self.position, targetPos, {
--     collisionType=nearby.COLLISION_TYPE.HeightMap + nearby.COLLISION_TYPE.Water,
--     radius = 10,
-- })

return nil

