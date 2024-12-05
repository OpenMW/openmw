---
-- `openmw.debug` is an interface to the engine debug utils.
-- Can be used only by local scripts, that are attached to a player.
-- @module Debug
-- @usage local debug = require('openmw.debug')


---
-- Rendering modes
-- @type RENDER_MODE
-- @field [parent=#RENDER_MODE] #number CollisionDebug
-- @field [parent=#RENDER_MODE] #number Wireframe
-- @field [parent=#RENDER_MODE] #number Pathgrid
-- @field [parent=#RENDER_MODE] #number Water
-- @field [parent=#RENDER_MODE] #number Scene
-- @field [parent=#RENDER_MODE] #number NavMesh
-- @field [parent=#RENDER_MODE] #number ActorsPaths
-- @field [parent=#RENDER_MODE] #number RecastMesh

---
-- Rendering mode values
-- @field [parent=#Debug] #RENDER_MODE RENDER_MODE

---
-- Toggles rendering mode
-- @function [parent=#Debug] toggleRenderMode
-- @param #RENDER_MODE value

---
-- Toggles god mode
-- @function [parent=#Debug] toggleGodMode

---
-- Is god mode enabled
-- @function [parent=#Debug] isGodMode
-- @return #boolean

---
-- Toggles AI
-- @function [parent=#Debug] toggleAI

---
-- Is AI enabled
-- @function [parent=#Debug] isAIEnabled
-- @return #boolean

---
-- Toggles collisions
-- @function [parent=#Debug] toggleCollision

---
-- Is player collision enabled
-- @function [parent=#Debug] isCollisionEnabled
-- @return #boolean

---
-- Toggles MWScripts
-- @function [parent=#Debug] toggleMWScript

---
-- Is MWScripts enabled
-- @function [parent=#Debug] isMWScriptEnabled
-- @return #boolean

---
-- Reloads all Lua scripts
-- @function [parent=#Debug] reloadLua

---
-- Navigation mesh rendering modes
-- @type NAV_MESH_RENDER_MODE
-- @field [parent=#NAV_MESH_RENDER_MODE] #number AreaType
-- @field [parent=#NAV_MESH_RENDER_MODE] #number UpdateFrequency

---
-- Navigation mesh rendering mode values
-- @field [parent=#Debug] #NAV_MESH_RENDER_MODE NAV_MESH_RENDER_MODE

---
-- Sets navigation mesh rendering mode
-- @function [parent=#Debug] setNavMeshRenderMode
-- @param #NAV_MESH_RENDER_MODE value

---
-- Enable/disable automatic reload of modified shaders
-- @function [parent=#Debug] setShaderHotReloadEnabled
-- @param #boolean value

---
-- To reload modified shaders
-- @function [parent=#Debug] triggerShaderReload
return nil
