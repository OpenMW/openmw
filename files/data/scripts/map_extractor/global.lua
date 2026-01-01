local util = require("openmw.util")
local world = require("openmw.world")
local async = require("openmw.async")

local visitedCells = {}

local cellCount = #world.cells
local i = cellCount


local function getExCellId(gridX, gridY)
    return string.format("(%d,%d)", gridX, gridY)
end


local function getCellId(cell)
    if not cell then return end
    if cell.isExterior then
        return getExCellId(cell.gridX, cell.gridY)
    else
        return (cell.id or ""):gsub(":", "")
    end
end


local function processAndTeleport(skipExtraction)
    local pl = world.players[1]

    if not skipExtraction then
        world.extractLocalMaps()
    end

    local function func()
        if world.isMapExtractionActive() then
            async:newUnsavableSimulationTimer(0.05, func)
            return
        elseif skipExtraction then
            pl:sendEvent("builtin:map_extractor:updateMenu", {
                line1 = "Generating local maps...",
            })
        end

        repeat
            local res, cell = pcall(function ()
                return world.cells[i]
            end)
            if not res then cell = nil end
            i = i - 1

            local pos

            local customCellId = getCellId(cell)
            if not cell or not customCellId or visitedCells[customCellId] then goto continue end

            visitedCells[customCellId] = true
            if cell.isExterior then
                for j = cell.gridX - 1, cell.gridX + 1 do
                    for k = cell.gridY - 1, cell.gridY + 1 do
                        visitedCells[getExCellId(j, k)] = true
                    end
                end
            end

            if cell.isExterior then
                pos = util.vector3(cell.gridX * 8192 + 4096, cell.gridY * 8192 + 4096, 0)
            else
                pos = util.vector3(0, 0, 0)
            end

            do
                pl:teleport(cell, pos)
                pl:sendEvent("builtin:map_extractor:updateMenu", {
                    line2 = string.format("Processed %d / %d cells", cellCount - i, cellCount),
                })
                break
            end

            ::continue::
        until i <= 0

        if i <= 0 then
            pl:sendEvent("builtin:map_extractor:updateMenu", {
                line1 = "Map extraction complete.",
                line2 = "",
            })
        end
    end

    async:newUnsavableSimulationTimer(0.05, func)
end


async:newUnsavableSimulationTimer(0.1, function ()
    world.players[1]:sendEvent("builtin:map_extractor:updateMenu", {line1 = "Generating world map..."})
    world.enableExtractionMode()
    world.extractWorldMap()

    if not world.getOverwriteFlag() then
        for _, cellId in pairs(world.getExistingLocalMapIds() or {}) do
            visitedCells[cellId] = true
        end
    end

    processAndTeleport(true)
end)



return {
    eventHandlers = {
        ["builtin:map_extractor:teleport"] = function (pl)
            processAndTeleport()
        end,
    }
}