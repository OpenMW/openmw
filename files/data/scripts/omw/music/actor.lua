local AI = require("openmw.interfaces").AI
local self = require("openmw.self")
local types = require("openmw.types")
local nearby = require("openmw.nearby")

local targets = {}

local function emitTargetsChanged()
    for _, actor in ipairs(nearby.players) do
        actor:sendEvent("OMWMusicCombatTargetsChanged", { actor = self, targets = targets })
    end
end

local function onUpdate()
    if types.Actor.isDeathFinished(self) or not types.Actor.isInActorsProcessingRange(self) then
        if next(targets) ~= nil then
            targets = {}
            emitTargetsChanged()
        end

        return
    end

    -- Early-out for actors without targets and without combat state
    -- TODO: use events or engine handlers to detect when targets change
    local isStanceNothing = types.Actor.getStance(self) == types.Actor.STANCE.Nothing
    if isStanceNothing and next(targets) == nil then
        return
    end

    local newTargets = AI.getTargets("Combat")

    local changed = false
    if #newTargets ~= #targets then
        changed = true
    else
        for i, target in ipairs(targets) do
            if target ~= newTargets[i] then
                changed = true
                break
            end
        end
    end

    targets = newTargets
    if changed then
        emitTargetsChanged()
    end
end

local function onInactive()
    if next(targets) ~= nil then
        targets = {}
        emitTargetsChanged()
    end
end

return {
    engineHandlers = {
        onUpdate = onUpdate,
        onInactive = onInactive,
    },
}
