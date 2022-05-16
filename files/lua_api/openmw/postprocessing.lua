---
-- `openmw.postprocessing` is an interface to postprocessing shaders.
-- Can be used only by local scripts, that are attached to a player.
-- @module shader
-- @usage local postprocessing = require('openmw.postprocessing')



---
-- Load a shader and return its handle.
-- @function [parent=#postprocessing] load
-- @param #string name Name of the shader without its extension
-- @return #Shader
-- @usage
-- If the shader exists and compiles, the shader will still be off by default.
-- It must be enabled to see its effect.
-- local vignetteShader = postprocessing.load('vignette')

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
-- @param #Vector2 value Value of uniform.

---
-- Set a non static Vector3 shader variable.
-- @function [parent=#Shader] setVector3
-- @param self
-- @param #string name Name of uniform
-- @param #Vector3 value Value of uniform.

---
-- Set a non static Vector4 shader variable.
-- @function [parent=#Shader] setVector4
-- @param self
-- @param #string name Name of uniform
-- @param #Vector4 value Value of uniform.


return nil
