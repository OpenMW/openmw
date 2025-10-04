local core = require('openmw.core')
local nearby = require('openmw.nearby')
local self = require('openmw.self')
local types = require('openmw.types')
local ui = require('openmw.ui')

local cell = nil
local autodoors = {}

local function onCellChange()
    autodoors = {}
    for _, door in ipairs(nearby.doors) do
        if door.type == types.ESM4Door and types.ESM4Door.record(door).isAutomatic then
            autodoors[#autodoors + 1] = door
        end
    end
end

local autodoorActivationDist = 300

local lastAutoActivation = 0
local function processAutomaticDoors()
    if core.getRealTime() - lastAutoActivation < 2 then
        return
    end
    for _, door in ipairs(autodoors) do
        if door.enabled and (door.position - self.position):length() < autodoorActivationDist then
            print('Automatic activation of', door)
            door:activateBy(self)
            lastAutoActivation = core.getRealTime()
        end
    end
end

local function onUpdate(dt)
    if dt <= 0 then
        return
    end

    if self.cell ~= cell then
        cell = self.cell
        onCellChange()
    end
    processAutomaticDoors()
end

return {
    engineHandlers = {
        onUpdate = onUpdate,
    },

    eventHandlers = {
        ShowMessage = function(data)
            if data.message then ui.showMessage(data.message) end
        end
    },
}
