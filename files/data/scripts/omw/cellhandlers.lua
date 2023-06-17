local types = require('openmw.types')
local util = require('openmw.util')
local world = require('openmw.world')

local CELL_SIZE = 8192

local function getRandomPosition(cellX, cellY)
    local x = math.random(7892)
    local y = math.random(7892)
    local z = -math.random(1748)
    return util.vector3(cellX + x, cellY + y, z)
end

local function getRandomOffset()
    local x = math.random(1000)
    local y = math.random(1000)
    local z = math.random(1000)
    local v = util.vector3(x, y, z)
    return v:normalize() * 100
end

local function getPlayerLevel()
    return types.Player.stats.level(world.players[1]).current
end

local function spawnFish(cell)
    if (cell.worldSpaceId ~= 'sys::default') then
        return
    end
    local spawnCount = math.random(0, 10)
    if (spawnCount < 1) then
        return
    end
    local list = types.LevelledCreature.record('h2o_all_lev-2')
    if (list == nil) then
        return
    end
    local cellX = cell.gridX * CELL_SIZE
    local cellY = cell.gridY * CELL_SIZE
    local level = getPlayerLevel()
    if (spawnCount <= 5) then -- spawn a number of random creatures selected from the list
        while(spawnCount > 0) do
            local id = list:getRandomId(level)
            if (id ~= '') then
                local ref = world.createObject(id)
                ref:teleport(cell, getRandomPosition(cellX, cellY))
            end
            spawnCount = spawnCount - 1
        end
    else -- spawn a horde of a single creature selected from the list
        local id = list:getRandomId(level)
        if (id ~= '') then
            local basePos = getRandomPosition(cellX, cellY)
            while(spawnCount > 0) do
                local ref = world.createObject(id)
                ref:teleport(cell, basePos + getRandomOffset())
                spawnCount = spawnCount - 1
            end
        end
    end
end

return {
    engineHandlers = { onNewExterior = spawnFish }
}
