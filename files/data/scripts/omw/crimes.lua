local types = require('openmw.types')
local world = require('openmw.world')
local I = require('openmw.interfaces')

local function isAllowedToOpen(player, lockable)
    -- Lockables are always allowed when their global variable is 1
    if lockable.globalVariable then
        local value = world.mwscript.getGlobalVariables()[lockable.globalVariable]
        if value and value >= 1 then
            return true
        end
    end

    local owner = lockable.owner
    local isOwned = false
    local isFactionOwned = false
    if owner.recordId and owner.recordId ~= player.recordId then
        isOwned = true
    end

    local faction = owner.factionId
    if faction then
        local rank = types.NPC.getFactionRank(player, owner.factionId)
        if rank == 0 then
            -- Not in faction.
            isFactionOwned = true
        else
            isFactionOwned = rank < (owner.factionRank or 0)
        end
    end

    return not(isFactionOwned or isOwned)
end

local function findOwner(object)
    if not object or not object.owner.recordId then
        return nil
    end
    for _, actor in pairs(world.activeActors) do
        if actor.recordId == object.owner.recordId then
            return actor
        end
    end
end

local function unlockAttempted(data)
    local player = data.player
    local lockable = data.lockable
    if not isAllowedToOpen(player, lockable) then
        I.Crimes.commitCrime(player, {
            player = player,
            type = types.Player.OFFENSE_TYPE.Trespassing,
            faction = lockable.owner.factionId,
            victim = findOwner(lockable),
        })
    end
end

---
-- Table with information needed to commit crimes.
-- @type CommitCrimeInputs
-- @field openmw.core#GameObject victim The victim of the crime (optional)
-- @field openmw.types#OFFENSE_TYPE_IDS type The type of the crime to commit. See @{openmw.types#OFFENSE_TYPE_IDS} (required)
-- @field #string faction ID of the faction the crime is committed against (optional)
-- @field #number arg The amount to increase the player bounty by if the crime type is theft. Ignored otherwise (optional, defaults to 0)
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
        UnlockAttempted = function(data) unlockAttempted(data) end
    }
}
