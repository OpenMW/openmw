-------------------------------------------------------------------------------
-- `openmw.util` defines utility functions and classes like 3D vectors, that don't depend on the game world.
-- @module util
-- @usage local util = require('openmw.util')



-------------------------------------------------------------------------------
-- Limits given value to the interval [`from`, `to`]
-- @function [parent=#util] clamp
-- @param #number value
-- @param #number from
-- @param #number to
-- @return #number min(max(value, from), to)

-------------------------------------------------------------------------------
-- Adds `2pi*k` and puts the angle in range `[-pi, pi]`
-- @function [parent=#util] normalizeAngle
-- @param #number angle Angle in radians
-- @return #number Angle in range `[-pi, pi]`


-------------------------------------------------------------------------------
-- Immutable 2D vector
-- @type Vector2
-- @field #number x
-- @field #number y
-- @usage
-- v = util.vector2(3, 4)
-- v.x, v.y       -- 3.0, 4.0
-- str(v)         -- "(3.0, 4.0)"
-- v:length()     -- 5.0    length
-- v:length2()    -- 25.0   square of the length
-- v:normalize()  -- vector2(3/5, 4/5)
-- v:rotate(radians)    -- rotate clockwise (returns rotated vector)
-- v1:dot(v2)     -- dot product (returns a number)
-- v1 * v2        -- dot product
-- v1 + v2        -- vector addition
-- v1 - v2        -- vector subtraction
-- v1 * x         -- multiplication by a number
-- v1 / x         -- division by a number

-------------------------------------------------------------------------------
-- Creates a new 2D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector2
-- @param #number x.
-- @param #number y.
-- @return #Vector2.

-------------------------------------------------------------------------------
-- Length of the vector
-- @function [parent=#Vector2] length
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Square of the length of the vector
-- @function [parent=#Vector2] length2
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Normalizes vector.
-- Returns two values: normalized vector and the length of the original vector.
-- It doesn't change the original vector. 
-- @function [parent=#Vector2] normalize
-- @param self
-- @return #Vector2, #number

-------------------------------------------------------------------------------
-- Rotates 2D vector clockwise.
-- @function [parent=#Vector2] rotate
-- @param self
-- @param #number angle Angle in radians
-- @return #Vector2 Rotated vector.

-------------------------------------------------------------------------------
-- Dot product.
-- @function [parent=#Vector2] dot
-- @param self
-- @param #Vector2 v
-- @return #number


-------------------------------------------------------------------------------
-- Immutable 3D vector
-- @type Vector3
-- @field #number x
-- @field #number y
-- @field #number z
-- @usage
-- v = util.vector3(3, 4, 5)
-- v.x, v.y, v.z  -- 3.0, 4.0, 5.0
-- str(v)         -- "(3.0, 4.0, 4.5)"
-- v:length()     -- length
-- v:length2()    -- square of the length
-- v:normalize()  -- normalized vector
-- v1:dot(v2)     -- dot product (returns a number)
-- v1 * v2        -- dot product (returns a number)
-- v1:cross(v2)   -- cross product (returns a vector)
-- v1 ^ v2        -- cross product (returns a vector)
-- v1 + v2        -- vector addition
-- v1 - v2        -- vector subtraction
-- v1 * x         -- multiplication by a number
-- v1 / x         -- division by a number

-------------------------------------------------------------------------------
-- Creates a new 3D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector3
-- @param #number x.
-- @param #number y.
-- @param #number z.
-- @return #Vector3.

-------------------------------------------------------------------------------
-- Length of the vector
-- @function [parent=#Vector3] length
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Square of the length of the vector
-- @function [parent=#Vector3] length2
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Normalizes vector.
-- Returns two values: normalized vector and the length of the original vector.
-- It doesn't change the original vector. 
-- @function [parent=#Vector3] normalize
-- @param self
-- @return #Vector3, #number

-------------------------------------------------------------------------------
-- Dot product.
-- @function [parent=#Vector3] dot
-- @param self
-- @param #Vector3 v
-- @return #number

-------------------------------------------------------------------------------
-- Cross product.
-- @function [parent=#Vector3] cross
-- @param self
-- @param #Vector3 v
-- @return #Vector3

return nil

