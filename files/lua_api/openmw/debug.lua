---
-- `openmw.debug` is an interface to the engine debug utils.
-- Can be used only by local scripts, that are attached to a player.
-- @module debug
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
-- @field [parent=#debug] #RENDER_MODE RENDER_MODE

---
-- Toggles rendering mode
-- @function [parent=#debug] toggleRenderMode
-- @param #RENDER_MODE value

---
-- Navigation mesh rendering modes
-- @type NAV_MESH_RENDER_MODE
-- @field [parent=#NAV_MESH_RENDER_MODE] #number AreaType
-- @field [parent=#NAV_MESH_RENDER_MODE] #number UpdateFrequency

---
-- Navigation mesh rendering mode values
-- @field [parent=#debug] #NAV_MESH_RENDER_MODE NAV_MESH_RENDER_MODE

---
-- Sets navigation mesh rendering mode
-- @function [parent=#debug] setNavMeshRenderMode
-- @param #NAV_MESH_RENDER_MODE value

return nil
