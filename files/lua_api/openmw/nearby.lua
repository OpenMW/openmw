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
-- Everything that can be picked up in the nearby.
-- @field [parent=#nearby] openmw.core#ObjectList items

---
-- @type COLLISION_TYPE
-- @field [parent=#COLLISION_TYPE] #number World
-- @field [parent=#COLLISION_TYPE] #number Door
-- @field [parent=#COLLISION_TYPE] #number Actor
-- @field [parent=#COLLISION_TYPE] #number HeightMap
-- @field [parent=#COLLISION_TYPE] #number Projectile
-- @field [parent=#COLLISION_TYPE] #number Water
-- @field [parent=#COLLISION_TYPE] #number Default Used by default: World+Door+Actor+HeightMap
-- @field [parent=#COLLISION_TYPE] #number AnyPhysical : World+Door+Actor+HeightMap+Projectile+Water
-- @field [parent=#COLLISION_TYPE] #number Camera Objects that should collide only with camera
-- @field [parent=#COLLISION_TYPE] #number VisualOnly Objects that were not intended to be part of the physics world

---
-- Collision types that are used in `castRay`.
-- Several types can be combined with '+'.
-- @field [parent=#nearby] #COLLISION_TYPE COLLISION_TYPE

---
-- Result of raycasing
-- @type RayCastingResult
-- @field [parent=#RayCastingResult] #boolean hit Is there a collision? (true/false)
-- @field [parent=#RayCastingResult] openmw.util#Vector3 hitPos Position of the collision point (nil if no collision)
-- @field [parent=#RayCastingResult] openmw.util#Vector3 hitNormal Normal to the surface in the collision point (nil if no collision)
-- @field [parent=#RayCastingResult] openmw.core#GameObject hitObject The object the ray has collided with (can be nil)

---
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

---
-- Cast ray from one point to another and find the first visual intersection with anything in the scene.
-- As opposite to `castRay` can find an intersection with an object without collisions.
-- In order to avoid threading issues can be used only in player scripts only in `onInputUpdate` or
-- in engine handlers for user input. In other cases use `asyncCastRenderingRay` instead.
-- @function [parent=#nearby] castRenderingRay
-- @param openmw.util#Vector3 from Start point of the ray.
-- @param openmw.util#Vector3 to End point of the ray.
-- @return #RayCastingResult

---
-- Asynchronously cast ray from one point to another and find the first visual intersection with anything in the scene.
-- @function [parent=#nearby] asyncCastRenderingRay
-- @param openmw.async#Callback callback The callback to pass the result to (should accept a single argument @{openmw.nearby#RayCastingResult}).
-- @param openmw.util#Vector3 from Start point of the ray.
-- @param openmw.util#Vector3 to End point of the ray.

return nil

