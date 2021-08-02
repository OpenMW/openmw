-------------------------------------------------------------------------------
-- `openmw.self` provides full access to the object the script is attached to.
-- Can be used only from local scripts. All fields and function of `GameObject` are also available for `openmw.self`.
-- @module self
-- @extends openmw.core#GameObject
-- @usage local self = require('openmw.self')
-- if self.type == 'Player' then  -- All fields and functions of `GameObject` are available.
--     self:sendEvent("something", self.position)
-- end



-------------------------------------------------------------------------------
-- Returns true if the script isActive (the object it is attached to is in an active cell).
-- If it is not active, then `openmw.nearby` can not be used.
-- @function [parent=#self] isActive
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- The object the script is attached to (readonly)
-- @field [parent=#self] openmw.core#GameObject object

-------------------------------------------------------------------------------
-- Movement controls (only for actors)
-- @field [parent=#self] #ActorControls controls

-------------------------------------------------------------------------------
-- Allows to view and/or modify controls of an actor. All fields are mutable.
-- @type ActorControls
-- @field [parent=#ActorControls] #number movement +1 - move forward, -1 - move backward
-- @field [parent=#ActorControls] #number sideMovement +1 - move right, -1 - move left
-- @field [parent=#ActorControls] #number turn Turn right (radians); if negative - turn left
-- @field [parent=#ActorControls] #boolean run true - run, false - walk
-- @field [parent=#ActorControls] #boolean jump If true - initiate a jump

-------------------------------------------------------------------------------
-- Enables or disables standart AI (enabled by default).
-- @function [parent=#self] enableAI
-- @param self
-- @param #boolean v

-------------------------------------------------------------------------------
-- Returns current target or nil if not in combat
-- @function [parent=#self] getCombatTarget
-- @param self
-- @return openmw.core#GameObject

-------------------------------------------------------------------------------
-- Remove all combat packages from the actor.
-- @function [parent=#self] stopCombat
-- @param self

-------------------------------------------------------------------------------
-- Attack `target`.
-- @function [parent=#self] startCombat
-- @param self
-- @param openmw.core#GameObject target

return nil

