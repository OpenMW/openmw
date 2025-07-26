local types = require('openmw.types')
local I = require('openmw.interfaces')

---
-- Table with information needed to commit crimes.
-- @type CommitCrimeInputs
-- @field openmw.core#GameObject victim The victim of the crime (optional)
-- @field openmw.types#OFFENSE_TYPE type The type of the crime to commit. See @{openmw.types#OFFENSE_TYPE} (required)
-- @field #string faction ID of the faction the crime is committed against (optional)
-- @field #number arg The amount to increase the player bounty by, if the crime type is theft. Ignored otherwise (optional, defaults to 0)
-- @field #boolean victimAware Whether the victim is aware of the crime (optional, defaults to false)

---
-- Table containing information returned by the engine after committing a crime
-- @type CommitCrimeOutputs
-- @field #boolean wasCrimeSeen Whether the crime was seen

return {
    interfaceName = 'Crimes',
    ---
    -- Allows to utilize built-in crime mechanics.
    -- @module Crimes
    -- @context global
    -- @usage require('openmw.interfaces').Crimes
    interface = {
        --- Interface version
        -- @field [parent=#Crimes] #number version
        version = 2,

        ---
        -- Commits a crime as if done through an in-game action. Can only be used in global context.
        -- @function [parent=#Crimes] commitCrime
        -- @param openmw.core#GameObject player The player committing the crime
        -- @param CommitCrimeInputs options A table of parameters describing the committed crime
        -- @return CommitCrimeOutputs A table containing information about the committed crime
        commitCrime = function(player, options)
            assert(types.Player.objectIsInstance(player), "commitCrime requires a player game object")

            local returnTable = {}
            local options = options or {}

            assert(type(options.faction) == "string" or options.faction == nil,
                "faction id passed to commitCrime must be a string or nil")
            assert(type(options.arg) == "number" or options.arg == nil,
                "arg value passed to commitCrime must be a number or nil")
            assert(type(options.victimAware) == "boolean" or options.victimAware == nil,
                "victimAware value passed to commitCrime must be a boolean or nil")

            assert(options.type ~= nil, "crime type passed to commitCrime cannot be nil")
            assert(type(options.type) == "number", "crime type passed to commitCrime must be a number")

            assert(options.victim == nil or types.NPC.objectIsInstance(options.victim),
                "victim passed to commitCrime must be an NPC or nil")

            returnTable.wasCrimeSeen = types.Player._runStandardCommitCrime(player, options.victim, options.type,
                options.faction or "",
                options.arg or 0, options.victimAware or false)
            return returnTable
        end,
    },
    eventHandlers = {
        CommitCrime = function(data) I.Crimes.commitCrime(data.player, data) end,
    }
}
