local weather = require('openmw.core').weather
local self = require('openmw.self')
local v3 = require('openmw.util').vector3

local redmountainPos = v3(25000, 70000, 0)

local function updateStormDirection(storm)
    if not storm or (storm.recordId ~= 'ashstorm' and storm.recordId ~= 'blight') then
        return
    end
    local playerPos = v3(self.position.x, self.position.y, 0)
    storm.stormDirection = (playerPos - redmountainPos):normalize()
end

local lastCell
local outside

return {
    engineHandlers = {
        onUpdate = function(dt)
            -- Skip work when the game is paused unless we teleported
            local cell = self.cell
            if lastCell ~= cell.id then
                lastCell = cell.id
                outside = cell.isExterior or cell:hasTag('QuasiExterior')
            elseif dt == 0 then
                return
            end
            if outside then
                updateStormDirection(weather.getCurrent(cell))
                updateStormDirection(weather.getNext(cell))
            end
        end
    }
}
