---
-- `openmw.postprocessing` is an interface to postprocessing shaders.
-- Can be used only by local scripts, that are attached to a player.
-- @module postprocessing
-- @usage local postprocessing = require('openmw.postprocessing')

---
-- @type Shader
-- @field #string name Name of the shader
-- @field #string description Description of the shader
-- @field #string author Author of the shader
-- @field #string version Version of the shader

---
-- Load a shader and return its handle.
-- @function [parent=#postprocessing] load
-- @param #string name Name of the shader without its extension
-- @return #Shader
-- @usage
-- -- If the shader exists and compiles, the shader will still be off by default.
-- -- It must be enabled to see its effect.
-- local vignetteShader = postprocessing.load('vignette')

---
-- Returns the ordered list of active shaders.
-- Active shaders may change between frames.
-- @function [parent=#postprocessing] getChain
-- @return #list<#Shader> list The currently active shaders, in order

---
-- Enable the shader. Has no effect if the shader is already enabled or does
-- not exist. Will not apply until the next frame.
-- @function [parent=#Shader] enable Enable the shader
-- @param self
-- @param #number position optional position to place the shader. If left out the shader will be inserted at the end of the chain.
-- @usage
-- -- Load shader
-- local vignetteShader = postprocessing.load('vignette')
-- -- Toggle shader on
-- vignetteShader:enable()

---
-- Deactivate the shader. Has no effect if the shader is already deactivated or does not exist.
-- Will not apply until the next frame.
-- @function [parent=#Shader] disable Disable the shader
-- @param self
-- @usage
-- local vignetteShader = shader.postprocessing('vignette')
-- vignetteShader:disable() -- shader will be toggled off

---
-- Check if the shader is enabled.
-- @function [parent=#Shader] isEnabled
-- @param self
-- @return #boolean True if shader is enabled and was compiled successfully.
-- @usage
-- local vignetteShader = shader.postprocessing('vignette')
-- vignetteShader:enable() -- shader will be toggled on

---
-- Set a non static bool shader variable.
-- @function [parent=#Shader] setBool
-- @param self
-- @param #string name Name of uniform
-- @param #boolean value Value of uniform.

---
-- Set a non static integer shader variable.
-- @function [parent=#Shader] setInt
-- @param self
-- @param #string name Name of uniform
-- @param #number value Value of uniform.

---
-- Set a non static float shader variable.
-- @function [parent=#Shader] setFloat
-- @param self
-- @param #string name Name of uniform
-- @param #number value Value of uniform.

---
-- Set a non static Vector2 shader variable.
-- @function [parent=#Shader] setVector2
-- @param self
-- @param #string name Name of uniform
-- @param openmw.util#Vector2 value Value of uniform.

---
-- Set a non static Vector3 shader variable.
-- @function [parent=#Shader] setVector3
-- @param self
-- @param #string name Name of uniform
-- @param openmw.util#Vector3 value Value of uniform.

---
-- Set a non static Vector4 shader variable.
-- @function [parent=#Shader] setVector4
-- @param self
-- @param #string name Name of uniform
-- @param openmw.util#Vector4 value Value of uniform.

---
-- Set a non static integer array shader variable.
-- @function [parent=#Shader] setIntArray
-- @param self
-- @param #string name Name of uniform
-- @param #table array Contains equal number of #number elements as the uniform array.

---
-- Set a non static float array shader variable.
-- @function [parent=#Shader] setFloatArray
-- @param self
-- @param #string name Name of uniform
-- @param #table array Contains equal number of #number elements as the uniform array.

---
-- Set a non static Vector2 array shader variable.
-- @function [parent=#Shader] setVector2Array
-- @param self
-- @param #string name Name of uniform
-- @param #table array Contains equal number of @{openmw.util#Vector2} elements as the uniform array.

---
-- Set a non static Vector3 array shader variable.
-- @function [parent=#Shader] setVector3Array
-- @param self
-- @param #string name Name of uniform
-- @param #table array Contains equal number of @{openmw.util#Vector3} elements as the uniform array.

---
-- Set a non static Vector4 array shader variable.
-- @function [parent=#Shader] setVector4Array
-- @param self
-- @param #string name Name of uniform
-- @param #table array Contains equal number of @{openmw.util#Vector4} elements as the uniform array.
-- @usage
-- -- Setting an array
-- local shader = postprocessing.load('godrays')
-- -- Toggle shader on
-- shader:enable()
-- -- Set new array uniform which was defined with length 2
-- shader:setVector4Array('myArray', { util.vector4(1,0,0,1), util.vector4(1,0,1,1) })

return nil
