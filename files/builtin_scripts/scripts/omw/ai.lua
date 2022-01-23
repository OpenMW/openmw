local self = require('openmw.self')
local interfaces = require('openmw.interfaces')

local function startPackage(args)
    if args.type == 'Combat' then
        if not args.target then error("target required") end
        self:_startAiCombat(args.target)
    elseif args.type == 'Pursue' then
        if not args.target then error("target required") end
        self:_startAiPursue(args.target)
    elseif args.type == 'Follow' then
        if not args.target then error("target required") end
        self:_startAiFollow(args.target)
    elseif args.type == 'Escort' then
        if not args.target then error("target required") end
        if not args.destPosition then error("destPosition required") end
        self:_startAiEscort(args.target, args.destCell or self.cell, args.duration or 0, args.destPosition)
    elseif args.type == 'Wander' then
        self:_startAiWander(args.distance or 0, args.duration or 0)
    elseif args.type == 'Travel' then
        if not args.destPosition then error("destPosition required") end
        self:_startAiTravel(args.destPosition)
    else
        error('Unsupported AI Package: '..args.type)
    end
end

local function filterPackages(filter)
    self:_iterateAndFilterAiSequence(filter)
end

return {
    interfaceName = 'AI',
    --- Basic AI interface
    -- @module AI
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
        -- @field openmw.util#Vector3 position Destination point of the AI package (can be nil).

        --- Return the currently active AI package (or `nil` if there are no AI packages).
        -- @function [parent=#AI] getActivePackage
        -- @return #Package
        getActivePackage = function() return self:_getActiveAiPackage() end,

        --- Start new AI package.
        -- @function [parent=#AI] startPackage
        -- @param #table options See the "Built-in AI packages" page.
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

        --- Get list of targets of all packages of the given type.
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

