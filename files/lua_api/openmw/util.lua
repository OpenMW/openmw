---
-- `openmw.util` defines utility functions and classes like 3D vectors, that don't depend on the game world.
-- @module util
-- @usage local util = require('openmw.util')



---
-- Limits given value to the interval [`from`, `to`].
-- @function [parent=#util] clamp
-- @param #number value
-- @param #number from
-- @param #number to
-- @return #number min(max(value, from), to)

---
-- Adds `2pi*k` and puts the angle in range `[-pi, pi]`.
-- @function [parent=#util] normalizeAngle
-- @param #number angle Angle in radians
-- @return #number Angle in range `[-pi, pi]`

---
-- Makes a table read only.
-- @function [parent=#util] makeReadOnly
-- @param #table table Any table.
-- @return #table The same table wrapped with read only userdata.


---
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
-- v:rotate(radians)    -- rotate counterclockwise (returns rotated vector)
-- v1:dot(v2)     -- dot product (returns a number)
-- v1 * v2        -- dot product
-- v1 + v2        -- vector addition
-- v1 - v2        -- vector subtraction
-- v1 * x         -- multiplication by a number
-- v1 / x         -- division by a number

---
-- Creates a new 2D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector2
-- @param #number x.
-- @param #number y.
-- @return #Vector2.

---
-- Length of the vector.
-- @function [parent=#Vector2] length
-- @param self
-- @return #number

---
-- Square of the length of the vector.
-- @function [parent=#Vector2] length2
-- @param self
-- @return #number

---
-- Normalizes vector.
-- Returns two values: normalized vector and the length of the original vector.
-- It doesn't change the original vector. 
-- @function [parent=#Vector2] normalize
-- @param self
-- @return #Vector2, #number

---
-- Rotates 2D vector clockwise.
-- @function [parent=#Vector2] rotate
-- @param self
-- @param #number angle Angle in radians
-- @return #Vector2 Rotated vector.

---
-- Dot product.
-- @function [parent=#Vector2] dot
-- @param self
-- @param #Vector2 v
-- @return #number


---
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

---
-- Creates a new 3D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector3
-- @param #number x.
-- @param #number y.
-- @param #number z.
-- @return #Vector3.

---
-- Length of the vector
-- @function [parent=#Vector3] length
-- @param self
-- @return #number

---
-- Square of the length of the vector
-- @function [parent=#Vector3] length2
-- @param self
-- @return #number

---
-- Normalizes vector.
-- Returns two values: normalized vector and the length of the original vector.
-- It doesn't change the original vector.
-- @function [parent=#Vector3] normalize
-- @param self
-- @return #Vector3, #number

---
-- Dot product.
-- @function [parent=#Vector3] dot
-- @param self
-- @param #Vector3 v
-- @return #number

---
-- Cross product.
-- @function [parent=#Vector3] cross
-- @param self
-- @param #Vector3 v
-- @return #Vector3


---
-- Immutable 4D vector.
-- @type Vector4
-- @field #number x
-- @field #number y
-- @field #number z
-- @field #number w
-- @usage
-- v = util.vector4(3, 4, 5, 6)
-- v.x, v.y, v.z, v.w  -- 3.0, 4.0, 5.0, 6.0
-- str(v)         -- "(3.0, 4.0, 5.0, 6.0)"
-- v:length()     -- length
-- v:length2()    -- square of the length
-- v:normalize()  -- normalized vector
-- v1:dot(v2)     -- dot product (returns a number)
-- v1 * v2        -- dot product (returns a number)
-- v1 + v2        -- vector addition
-- v1 - v2        -- vector subtraction
-- v1 * x         -- multiplication by a number
-- v1 / x         -- division by a number

---
-- Creates a new 4D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector4
-- @param #number x.
-- @param #number y.
-- @param #number z.
-- @param #number w.
-- @return #Vector4.

---
-- Length of the vector
-- @function [parent=#Vector4] length
-- @param self
-- @return #number

---
-- Square of the length of the vector
-- @function [parent=#Vector4] length2
-- @param self
-- @return #number

---
-- Normalizes vector.
-- Returns two values: normalized vector and the length of the original vector.
-- It doesn't change the original vector.
-- @function [parent=#Vector4] normalize
-- @param self
-- @return #Vector4, #number

---
-- Dot product.
-- @function [parent=#Vector4] dot
-- @param self
-- @param #Vector4 v
-- @return #number

---
-- Color in RGBA format. All of the component values are in the range [0, 1].
-- @type Color
-- @field #number r Red component
-- @field #number g Green component
-- @field #number b Blue component
-- @field #number a Alpha (transparency) component

---
-- Returns a Vector4 with RGBA components of the Color.
-- @function [parent=#Color] asRgba
-- @param self
-- @return #Vector4

---
-- Returns a Vector3 with RGB components of the Color.
-- @function [parent=#Color] asRgb
-- @param self
-- @return #Vector3

---
-- Converts the color into a HEX string.
-- @function [parent=#Color] asHex
-- @param self
-- @return #string

---
-- Methods for creating #Color values from different formats.
-- @type COLOR

---
-- Methods for creating #Color values from different formats.
-- @field [parent=#util] #COLOR color

---
-- Creates a Color from RGBA format
-- @function [parent=#COLOR] rgba
-- @param #number r
-- @param #number g
-- @param #number b
-- @param #number a
-- @return #Color

---
-- Creates a Color from RGB format. Equivalent to calling util.rgba with a = 1.
-- @function [parent=#COLOR] rgb
-- @param #number r
-- @param #number g
-- @param #number b
-- @return #Color

---
-- Parses a hex color string into a Color.
-- @function [parent=#COLOR] hex
-- @param #string hex A hex color string in RRGGBB format (e. g. "ff0000").
-- @return #Color

---
-- @type Transform

---
-- Returns the inverse transform.
-- @function [parent=#Transform] inverse
-- @param self
-- @return #Transform.

---
-- @type TRANSFORM
-- @field [parent=#TRANSFORM] #Transform identity Empty transform.

---
-- Movement by given vector.
-- @function [parent=#TRANSFORM] move
-- @param #Vector3 offset.
-- @return #Transform.
-- @usage
-- -- Accepts either 3 numbers or a 3D vector
-- util.transform.move(x, y, z)
-- util.transform.move(util.vector3(x, y, z))

---
-- Scale transform.
-- @function [parent=#TRANSFORM] scale
-- @param #number scaleX.
-- @param #number scaleY.
-- @param #number scaleZ.
-- @return #Transform.
-- @usage
-- -- Accepts either 3 numbers or a 3D vector
-- util.transform.scale(x, y, z)
-- util.transform.scale(util.vector3(x, y, z))


---
-- Rotation around a vector (counterclockwise if the vector points to us).
-- @function [parent=#TRANSFORM] rotate
-- @param #number angle
-- @param #Vector3 axis.
-- @return #Transform.

---
-- X-axis rotation (equivalent to `rotate(angle, vector3(-1, 0, 0))`).
-- @function [parent=#TRANSFORM] rotateX
-- @param #number angle
-- @return #Transform.

---
-- Y-axis rotation (equivalent to `rotate(angle, vector3(0, -1, 0))`).
-- @function [parent=#TRANSFORM] rotateY
-- @param #number angle
-- @return #Transform.

---
-- Z-axis rotation (equivalent to `rotate(angle, vector3(0, 0, -1))`).
-- @function [parent=#TRANSFORM] rotateZ
-- @param #number angle
-- @return #Transform.

---
-- 3D transforms (scale/move/rotate) that can be applied to 3D vectors.
-- Several transforms can be combined and applied to a vector using multiplication.
-- Combined transforms apply in reverse order (from right to left).
-- @field [parent=#util] #TRANSFORM transform
-- @usage
-- local util = require('openmw.util')
-- local trans = util.transform
-- local fromActorSpace = trans.move(actor.position) * trans.rotateZ(actor.rotation.z)
--
-- -- rotation is applied first, movement is second
-- local posBehindActor = fromActorSpace * util.vector3(0, -100, 0)
--
-- -- equivalent to trans.rotateZ(-actor.rotation.z) * trans.move(-actor.position)
-- local toActorSpace = fromActorSpace:inverse()
-- local relativeTargetPos = toActorSpace * target.position
-- local deltaAngle = math.atan2(relativeTargetPos.y, relativeTargetPos.x)

return nil

