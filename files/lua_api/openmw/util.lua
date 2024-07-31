---
-- `openmw.util` defines utility functions and classes like 3D vectors, that don't depend on the game world.
-- @module util
-- @usage local util = require('openmw.util')


---
-- Rounds the given value to the nearest whole number.
-- @function [parent=#util] round
-- @param #number value
-- @return #number The rounded value.
-- @usage
-- local util = require('openmw.util')
-- local roundedValue = util.round(3.141592)
-- print(roundedValue) -- prints 3

---
-- Remaps the value from one range to another.
-- @function [parent=#util] remap
-- @param #number value
-- @param #number min
-- @param #number max
-- @param #number newMin
-- @param #number newMax
-- @return #number The remapped value.
-- @usage
-- local util = require('openmw.util')
-- local newValue = util.remap(3, 0, 10, 0, 100)
-- print(newValue) -- prints 30

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
-- Makes a table read only and overrides `__index` with the strict version that throws an error if the key is not found.
-- @function [parent=#util] makeStrictReadOnly
-- @param #table table Any table.
-- @return #table The same table wrapped with read only userdata.

---
-- Parses Lua code from string and returns as a function.
-- @function [parent=#util] loadCode
-- @param #string code Lua code.
-- @param #table table Environment to run the code in.
-- @return #function The loaded code.

---
-- Bitwise And (supports any number of arguments).
-- @function [parent=#util] bitAnd
-- @param #number A First argument (integer).
-- @param #number B Second argument (integer).
-- @return #number Bitwise And of A and B.

---
-- Bitwise Or (supports any number of arguments).
-- @function [parent=#util] bitOr
-- @param #number A First argument (integer).
-- @param #number B Second argument (integer).
-- @return #number Bitwise Or of A and B.

---
-- Bitwise Xor (supports any number of arguments).
-- @function [parent=#util] bitXor
-- @param #number A First argument (integer).
-- @param #number B Second argument (integer).
-- @return #number Bitwise Xor of A and B.

---
-- Bitwise inversion.
-- @function [parent=#util] bitNot
-- @param #number A Argument (integer).
-- @return #number Bitwise Not of A.


---
-- Immutable 2D vector
-- @type Vector2
-- @field #number x
-- @field #number y
-- @field #string xy swizzle support, any combination of fields can be used to construct a new vector
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
-- v1.xx, v1.xyx  -- new vectors can be created with swizzles

---
-- Creates a new 2D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector2
-- @param #number x.
-- @param #number y.
-- @return #Vector2.

---
-- @function [parent=#Vector2] __add
-- @param self
-- @param #Vector2 v
-- @return #Vector2 sum of the vectors

---
-- @function [parent=#Vector2] __sub
-- @param self
-- @param #Vector2 v
-- @return #Vector2 difference of the vectors

---
-- @function [parent=#Vector2] __mul
-- @param self
-- @param #number k
-- @return #Vector2 vector multiplied by a number

---
-- @function [parent=#Vector2] __div
-- @param self
-- @param #number k
-- @return #Vector2 vector divided by a number

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
-- It doesn't change the original vector.
-- @function [parent=#Vector2] normalize
-- @param self
-- @return #Vector2 normalized vector
-- @return #number the length of the original vector

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
-- Element-wise multiplication
-- @function [parent=#Vector2] emul
-- @param self
-- @param #Vector2 v
-- @return #Vector2

---
-- Element-wise division
-- @function [parent=#Vector2] ediv
-- @param self
-- @param #Vector2 v
-- @return #Vector2


---
-- Immutable 3D vector
-- @type Vector3
-- @field #number x
-- @field #number y
-- @field #number z
-- @field #string xyz swizzle support, any combination of fields can be used to construct a new vector
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
-- v1.zyz, v1.yx  -- new vectors can be created with swizzles

---
-- Creates a new 3D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector3
-- @param #number x.
-- @param #number y.
-- @param #number z.
-- @return #Vector3.

---
-- @function [parent=#Vector3] __add
-- @param self
-- @param #Vector3 v
-- @return #Vector3 sum of the vectors

---
-- @function [parent=#Vector3] __sub
-- @param self
-- @param #Vector3 v
-- @return #Vector3 difference of the vectors

---
-- @function [parent=#Vector3] __mul
-- @param self
-- @param #number k
-- @return #Vector3 vector multiplied by a number

---
-- @function [parent=#Vector3] __div
-- @param self
-- @param #number k
-- @return #Vector3 vector divided by a number

---
-- @function [parent=#Vector3] __tostring
-- @param self
-- @return #string

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
-- It doesn't change the original vector.
-- @function [parent=#Vector3] normalize
-- @param self
-- @return #Vector3 normalized vector
-- @return #number the length of the original vector

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
-- Element-wise multiplication
-- @function [parent=#Vector3] emul
-- @param self
-- @param #Vector3 v
-- @return #Vector3

---
-- Element-wise division
-- @function [parent=#Vector3] ediv
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
-- @field #string xyzw swizzle support, any combination of fields can be used to construct a new vector
-- @usage
-- v = util.vector4(3, 4, 5, 6)
-- v.x, v.y, v.z, v.w  -- 3.0, 4.0, 5.0, 6.0
-- str(v)           -- "(3.0, 4.0, 5.0, 6.0)"
-- v:length()       -- length
-- v:length2()      -- square of the length
-- v:normalize()    -- normalized vector
-- v1:dot(v2)       -- dot product (returns a number)
-- v1 * v2          -- dot product (returns a number)
-- v1 + v2          -- vector addition
-- v1 - v2          -- vector subtraction
-- v1 * x           -- multiplication by a number
-- v1 / x           -- division by a number
-- v1.zzzz, v1.zyz  -- new vectors can be created with swizzles

---
-- Creates a new 4D vector. Vectors are immutable and can not be changed after creation.
-- @function [parent=#util] vector4
-- @param #number x.
-- @param #number y.
-- @param #number z.
-- @param #number w.
-- @return #Vector4.

---
-- @function [parent=#Vector4] __add
-- @param self
-- @param #Vector4 v
-- @return #Vector4 sum of the vectors

---
-- @function [parent=#Vector4] __sub
-- @param self
-- @param #Vector4 v
-- @return #Vector4 difference of the vectors

---
-- @function [parent=#Vector4] __mul
-- @param self
-- @param #number k
-- @return #Vector4 vector multiplied by a number

---
-- @function [parent=#Vector4] __div
-- @param self
-- @param #number k
-- @return #Vector4 vector divided by a number

---
-- @function [parent=#Vector4] __tostring
-- @param self
-- @return #string

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
-- It doesn't change the original vector.
-- @function [parent=#Vector4] normalize
-- @param self
-- @return #Vector4 normalized vector
-- @return #number the length of the original vector

---
-- Dot product.
-- @function [parent=#Vector4] dot
-- @param self
-- @param #Vector4 v
-- @return #number

---
-- Element-wise multiplication
-- @function [parent=#Vector4] emul
-- @param self
-- @param #Vector4 v
-- @return #Vector4

---
-- Element-wise division
-- @function [parent=#Vector4] ediv
-- @param self
-- @param #Vector4 v
-- @return #Vector4

---
-- Immutable box.
-- @type Box
-- @field #Vector3 center The center of the box
-- @field #Vector3 halfSize The half sizes of the box along each axis
-- @field #Transform transform A transformation which encapsulates the boxes center pointer (translation), half sizes (scale), and rotation.
-- @field #table vertices Table of the 8 vertices which comprise the box, taking rotation into account

---
-- Creates a new Box with a given center and half sizes. Boxes are immutable and can not be changed after creation.
-- @function [parent=#util] box
-- @param #Vector3 center
-- @param #Vector3 halfSize in each dimension (x, y, z)
-- @return #Box

---
-- Creates a new Box from a given transformation. Boxes are immutable and can not be changed after creation.
-- @function [parent=#util] box
-- @param #Transform transform A transformation which encapsulates the boxes center pointer (translation), half sizes (scale), and rotation.
-- @return #Box
-- @usage
-- -- Creates a 1x1x1 length box centered at the origin
-- util.box(util.transform.scale(util.vector3(0.5, 0.5, 0.5)))

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
-- Combine transforms (will apply in reverse order)
-- @function [parent=#Transform] __mul
-- @param self
-- @param #Transform t
-- @return #Transform

---
-- Returns the inverse transform.
-- @function [parent=#Transform] inverse
-- @param self
-- @return #Transform

---
-- Apply transform to a vector
-- @function [parent=#Transform] apply
-- @param self
-- @param #Vector3 v
-- @return #Vector3

---
-- Get yaw angle (radians)
-- @function [parent=#Transform] getYaw
-- @param self
-- @return #number

---
-- Get pitch angle (radians)
-- @function [parent=#Transform] getPitch
-- @param self
-- @return #number

---
-- Get Euler angles for XZ rotation order (pitch and yaw; radians)
-- @function [parent=#Transform] getAnglesXZ
-- @param self
-- @return #number pitch (rotation around X axis)
-- @return #number yaw (rotation around Z axis)

---
-- Get Euler angles for ZYX rotation order (radians)
-- @function [parent=#Transform] getAnglesZYX
-- @param self
-- @return #number rotation around Z axis (first rotation)
-- @return #number rotation around Y axis (second rotation)
-- @return #number rotation around X axis (third rotation)

---
-- @type TRANSFORM
-- @field [parent=#TRANSFORM] #Transform identity Empty transform.

---
-- Movement by given vector.
-- @function [parent=#TRANSFORM] move
-- @param #Vector3 offset
-- @return #Transform
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
-- local fromActorSpace = trans.move(actor.position) * trans.rotateZ(actor.rotation:getYaw())
--
-- -- rotation is applied first, movement is second
-- local posBehindActor = fromActorSpace * util.vector3(0, -100, 0)
--
-- -- equivalent to trans.rotateZ(-actor.rotation:getYaw()) * trans.move(-actor.position)
-- local toActorSpace = fromActorSpace:inverse()
-- local relativeTargetPos = toActorSpace * target.position
-- local deltaAngle = math.atan2(relativeTargetPos.y, relativeTargetPos.x)

return nil
