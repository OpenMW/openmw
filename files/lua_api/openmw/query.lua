-------------------------------------------------------------------------------
-- `openmw.query` constructs queries that can be used in `world.selectObjects` and `nearby.selectObjects`.
-- @module query
-- @usage local query = require('openmw.query')



-------------------------------------------------------------------------------
-- Query. A Query itself can no return objects. It only holds search conditions.
-- @type Query

-------------------------------------------------------------------------------
-- Add condition.
-- @function [parent=#Query] where
-- @param self
-- @param condition
-- @return #Query

-------------------------------------------------------------------------------
-- Limit result size.
-- @function [parent=#Query] limit
-- @param self
-- @param #number maxCount
-- @return #Query


-------------------------------------------------------------------------------
-- A field that can be used in a condition
-- @type Field

-------------------------------------------------------------------------------
-- Equal
-- @function [parent=#Field] eq
-- @param self
-- @param value

-------------------------------------------------------------------------------
-- Not equal
-- @function [parent=#Field] neq
-- @param self
-- @param value

-------------------------------------------------------------------------------
-- Lesser
-- @function [parent=#Field] lt
-- @param self
-- @param value

-------------------------------------------------------------------------------
-- Lesser or equal
-- @function [parent=#Field] lte
-- @param self
-- @param value

-------------------------------------------------------------------------------
-- Greater
-- @function [parent=#Field] gt
-- @param self
-- @param value

-------------------------------------------------------------------------------
-- Greater or equal
-- @function [parent=#Field] gte
-- @param self
-- @param value


-------------------------------------------------------------------------------
-- @type OBJECT
-- @field [parent=#OBJECT] #Field type
-- @field [parent=#OBJECT] #Field recordId
-- @field [parent=#OBJECT] #Field count
-- @field [parent=#OBJECT] #CellFields cell

-------------------------------------------------------------------------------
-- Fields that can be used with any object.
-- @field [parent=#query] #OBJECT OBJECT

-------------------------------------------------------------------------------
-- @type DOOR
-- @field [parent=#DOOR] #Field isTeleport
-- @field [parent=#DOOR] #CellFields destCell

-------------------------------------------------------------------------------
-- Fields that can be used only when search for doors.
-- @field [parent=#query] #DOOR DOOR

-------------------------------------------------------------------------------
-- @type CellFields
-- @field [parent=#CellFields] #Field name
-- @field [parent=#CellFields] #Field region
-- @field [parent=#CellFields] #Field isExterior


-------------------------------------------------------------------------------
-- Base Query to select activators.
-- @field [parent=#query] #Query activators

-------------------------------------------------------------------------------
-- Base Query to select actors.
-- @field [parent=#query] #Query actors

-------------------------------------------------------------------------------
-- Base Query to select containers.
-- @field [parent=#query] #Query containers

-------------------------------------------------------------------------------
-- Base Query to select doors.
-- @field [parent=#query] #Query doors

-------------------------------------------------------------------------------
-- Base Query to select items.
-- @field [parent=#query] #Query items

return nil

