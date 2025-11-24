local self = require('openmw.self')
local interfaces = require('openmw.interfaces')
local types = require('openmw.types')
local util = require('openmw.util')

local function startPackage(args)
    local cancelOther = args.cancelOther
    if cancelOther == nil then cancelOther = true end
    if args.type == 'Combat' then
        if not args.target then error("target required") end
        self:_startAiCombat(args.target, cancelOther)
    elseif args.type == 'Pursue' then
        if not args.target then error("target required") end
        if not types.Player.objectIsInstance(args.target) then error("target must be a player") end
        self:_startAiPursue(args.target, cancelOther)
    elseif args.type == 'Follow' then
        if not args.target then error("target required") end
        self:_startAiFollow(args.target, args.cellId, args.duration or 0, args.destPosition or util.vector3(0, 0, 0), args.isRepeat or false, cancelOther)
    elseif args.type == 'Escort' then
        if not args.target then error("target required") end
        if not args.destPosition then error("destPosition required") end
        self:_startAiEscort(args.target, args.destCell or self.cell, args.duration or 0, args.destPosition, cancelOther)
    elseif args.type == 'Wander' then
        local key = "idle"
        local idle = {}
        local duration = 0
        if args.idle then
            for i = 2, 9 do
                local val = args.idle[key .. i]
                if val == nil then
                    idle[i-1] = 0
                else
                    local v = tonumber(val) or 0
                    if v < 0 or v > 100 then
                        error("idle values cannot exceed 100")
                    end
                    idle[i-1] = v
                end
            end
        end
        if args.duration then
            duration = args.duration / 3600
        end
        self:_startAiWander(args.distance or 0, duration, idle, args.isRepeat or false, cancelOther)
    elseif args.type == 'Travel' then
        if not args.destPosition then error("destPosition required") end
        self:_startAiTravel(args.destPosition, args.isRepeat or false, cancelOther)
    else
        error('Unsupported AI Package: ' .. args.type)
    end
end

local function filterPackages(filter)
    self:_iterateAndFilterAiSequence(filter)
end

return {
    interfaceName = 'AI',
    --- Basic AI interface
    -- @module AI
    -- @context local
    -- @usage require('openmw.interfaces').AI
    interface = {
        --- Interface version
        -- @field [parent=#AI] #number version
        version = 0,

        --- AI Package
        -- @type Package
        -- @field #string type Type of the AI package.
        -- @field openmw.core#GameObject target Target (usually an actor) of the AI package (can be nil).
        -- @field #boolean sideWithTarget Whether to help the target in combat (true or false).
        -- @field openmw.util#Vector3 destPosition Destination point of the AI package.
        -- @field #number distance Distance value (can be nil).
        -- @field #number duration Duration value (can be nil).
        -- @field #table idle Idle value (can be nil).
        -- @field #boolean isRepeat Should this package be repeated (true or false).

        --- Return the currently active AI package (or `nil` if there are no AI packages).
        -- @function [parent=#AI] getActivePackage
        -- @return #Package
        getActivePackage = function() return self:_getActiveAiPackage() end,

        --- Return whether the actor is fleeing.
        -- @function [parent=#AI] isFleeing
        -- @return #boolean
        isFleeing = function() return self:_isFleeing() end,

        --- Start a new AI package.
        -- @function [parent=#AI] startPackage
        -- @param #table options See the "AI packages" page.
        startPackage = startPackage,

        --- Iterate over all packages starting from the active one and remove those where `filterCallback` returns false.
        -- @function [parent=#AI] filterPackages
        -- @param #function filterCallback
        filterPackages = filterPackages,

        --- Iterate over all packages and run `callback` for each starting from the active one.
        -- The same as `filterPackage`, but without removal.
        -- @function [parent=#AI] forEachPackage
        -- @param #function callback
        forEachPackage = function(callback)
            local filter = function(p)
                callback(p)
                return true
            end
            filterPackages(filter)
        end,

        --- Remove packages of given type (remove all packages if the type is not specified).
        -- @function [parent=#AI] removePackages
        -- @param #string packageType (optional) The type of packages to remove.
        removePackages = function(packageType)
            filterPackages(function(p) return packageType and p.type ~= packageType end)
        end,

        --- Return the target of the active package if the package has given type
        -- @function [parent=#AI] getActiveTarget
        -- @param #string packageType The expected type of the active package
        -- @return openmw.core#GameObject The target (can be nil if the package has no target or has another type)
        getActiveTarget = function(packageType)
            local p = self:_getActiveAiPackage()
            if p and p.type == packageType then
                return p.target
            else
                return nil
            end
        end,

        --- Get a list of targets from all packages of the given type.
        -- @function [parent=#AI] getTargets
        -- @param #string packageType
        -- @return #list<openmw.core#GameObject>
        getTargets = function(packageType)
            local res = {}
            filterPackages(function(p)
                if p.type == packageType and p.target then
                    res[#res + 1] = p.target
                end
                return true
            end)
            return res
        end,
    },
    eventHandlers = {
        StartAIPackage = function(options) interfaces.AI.startPackage(options) end,
        RemoveAIPackages = function(packageType) interfaces.AI.removePackages(packageType) end,
    },
}
