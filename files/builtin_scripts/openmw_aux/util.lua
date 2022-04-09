---
-- `openmw_aux.util` defines utility functions that are implemented in Lua rather than in C++.
-- Implementation can be found in `resources/vfs/openmw_aux/util.lua`.
-- @module util
-- @usage local aux_util = require('openmw_aux.util')

local aux_util = {}

---
-- Works like `tostring` but shows also content of tables.
-- @function [parent=#util] deepToString
-- @param #any value The value to conver to string
-- @param #number maxDepth Max depth of tables unpacking (optional, 2 by default)
function aux_util.deepToString(val, level, prefix)
    level = (level or 1) - 1
    local ok, iter, t = pcall(function() return pairs(val) end)
    if level < 0 or not ok then
        return tostring(val)
    end
    local prefix = prefix or ''
    local newPrefix = prefix .. '  '
    local strs = {tostring(val) .. ' {\n'}
    for k, v in iter, t do
        strs[#strs + 1] = newPrefix .. tostring(k) .. ' = ' .. aux_util.deepToString(v, level, newPrefix) .. ',\n'
    end
    strs[#strs + 1] = prefix .. '}'
    return table.concat(strs)
end

-------------------------------------------------------------------------------
-- Finds the nearest object to the given point in the given list.
-- Ignores cells, uses only coordinates. Returns the nearest object,
-- and the distance to it. If objectList is empty, returns nil.
-- @function [parent=#util] findNearestTo
-- @param openmw.util#Vector3 point
-- @param openmw.core#ObjectList objectList
-- @param #number minDist (optional) ignore objects that are closer than minDist
-- @param #number maxDist (optional) ignore objects that are farther than maxDist
-- @return openmw.core#GameObject, #number the nearest object and the distance
function aux_util.findNearestTo(point, objectList, minDist, maxDist)
    local res = nil
    local resDist = nil
    local minDist = minDist or 0
    for i = 1, #objectList do
        local obj = objectList[i]
        local dist = (obj.position - point):length()
        if dist >= minDist and (not res or dist < resDist) then
            res = obj
            resDist = dist
        end
    end
    if res and (not maxDist or resDist <= maxDist) then
        return res, resDist
    end
end

return aux_util

