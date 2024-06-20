---
-- `openmw.self` provides full access to the object the script is attached to.
-- Can be used only from local scripts. All fields and function of `GameObject` are also available for `openmw.self`.
-- @module self
-- @extends openmw.core#GameObject
-- @usage local self = require('openmw.self')
-- local types = require('openmw.types')
-- if self.type == types.Player then  -- All fields and functions of `GameObject` are available.
--     self:sendEvent("something", self.position)
-- end



---
-- Returns true if the script isActive (the object it is attached to is in an active cell).
-- If it is not active, then `openmw.nearby` can not be used.
-- @function [parent=#self] isActive
-- @param self
-- @return #boolean

---
-- The object the script is attached to (readonly)
-- @field [parent=#self] openmw.core#GameObject object


---
-- Movement controls (only for actors)
-- @field [parent=#self] #ActorControls controls

---
-- Allows to view and/or modify controls of an actor. All fields are mutable.
-- @type ActorControls
-- @field [parent=#ActorControls] #number movement +1 - move forward, -1 - move backward
-- @field [parent=#ActorControls] #number sideMovement +1 - move right, -1 - move left
-- @field [parent=#ActorControls] #number yawChange Turn right (radians); if negative - turn left
-- @field [parent=#ActorControls] #number pitchChange Look down (radians); if negative - look up
-- @field [parent=#ActorControls] #boolean run true - run, false - walk
-- @field [parent=#ActorControls] #boolean sneak If true - sneak
-- @field [parent=#ActorControls] #boolean jump If true - initiate a jump
-- @field [parent=#ActorControls] #number use Possible values: 0 - 3. If above 0 - activates the readied weapon/spell. For weapons, keeping at a non-zero value will charge the attack until set to 0. For melee weapons on AI-disabled actors - values 1 - 3 determine an attack type: 1 - chop, 2 - slash, 3 - thrust.

---
-- Enables or disables standard AI (enabled by default).
-- @function [parent=#self] enableAI
-- @param self
-- @param #boolean v

return nil
