---
-- `openmw_aux.util` defines utility functions that are implemented in Lua rather than in C++.
-- Implementation can be found in `resources/vfs/openmw_aux/util.lua`.
-- @module util
-- @context global|menu|local
-- @usage local aux_util = require('openmw_aux.util')

local aux_util = {}

local function deepToString(val, level, prefix)
    local level = (level or 1) - 1
    local ok, iter, t = pcall(function() return pairs(val) end)
    if level < 0 or not ok then
        return tostring(val)
    end
    local newPrefix = prefix .. '  '
    local strs = {tostring(val) .. ' {\n'}
    for k, v in iter, t do
        strs[#strs + 1] = newPrefix .. tostring(k) .. ' = ' .. deepToString(v, level, newPrefix) .. ',\n'
    end
    strs[#strs + 1] = prefix .. '}'
    return table.concat(strs)
end

---
-- Works like `tostring` but shows also content of tables.
-- @function [parent=#util] deepToString
-- @param #any value The value to convert to string
-- @param #number maxDepth Max depth of tables unpacking (optional, 1 by default)
function aux_util.deepToString(value, maxDepth)
    return deepToString(value, maxDepth, '')
end

---
-- Finds the element the minimizes `scoreFn`.
-- @function [parent=#util] findMinScore
-- @param #table array Any array
-- @param #function scoreFn Function that returns either nil/false or a number for each element of the array
-- @return element The element the minimizes `scoreFn`
-- @return #number score The output of `scoreFn(element)`
-- @return #number index The index of the chosen element in the array
-- @usage -- Find the nearest NPC
-- local nearestNPC, distToNPC = aux_util.findMinScore(
--     nearby.actors,
--     function(actor)
--         return actor.type == types.NPC and (self.position - actor.position):length()
--     end)
function aux_util.findMinScore(array, scoreFn)
    local bestValue, bestScore, bestIndex
    for i = 1, #array do
        local v = array[i]
        local score = scoreFn(v)
        if score and (not bestScore or bestScore > score) then
            bestValue, bestScore, bestIndex = v, score, i
        end
    end
    return bestValue, bestScore, bestIndex
end

---
-- Computes `scoreFn` for each element of `array` and filters out elements with false and nil results.
-- @function [parent=#util] mapFilter
-- @param #table array Any array
-- @param #function scoreFn Filter function
-- @return #table Output array
-- @return #table Array of the same size with corresponding scores
-- @usage -- Find all NPCs in `nearby.actors`
-- local NPCs = aux_util.mapFilter(
--     nearby.actors,
--     function(actor) return actor.type == types.NPC end)
function aux_util.mapFilter(array, scoreFn)
    local res = {}
    local scores = {}
    for i = 1, #array do
        local v = array[i]
        local f = scoreFn(v)
        if f then
            scores[#res + 1] = f
            res[#res + 1] = v
        end
    end
    return res, scores
end

---
-- Filters and sorts `array` by the scores calculated by `scoreFn`. The same as `aux_util.mapFilter`, but the result is sorted.
-- @function [parent=#util] mapFilterSort
-- @param #table array Any array
-- @param #function scoreFn Filter function
-- @return #table Output array
-- @return #table Array of the same size with corresponding scores
-- @usage -- Find all NPCs in `nearby.actors` and sort them by distances
-- local NPCs, distances = aux_util.mapFilterSort(
--     nearby.actors,
--     function(actor)
--         return actor.type == types.NPC and (self.position - actor.position):length()
--     end)
function aux_util.mapFilterSort(array, scoreFn)
    local values, scores = aux_util.mapFilter(array, scoreFn)
    local size = #values
    local ids = {}
    for i = 1, size do ids[i] = i end
    table.sort(ids, function(i, j) return scores[i] < scores[j] end)
    local sortedValues = {}
    local sortedScores = {}
    for i = 1, size do
        sortedValues[i] = values[ids[i]]
        sortedScores[i] = scores[ids[i]]
    end
    return sortedValues, sortedScores
end

---
-- Iterates over an array of event handlers, calling each in turn until one returns false.
-- @function [parent=#util] callEventHandlers
-- @param #table handlers An optional array of handlers to invoke
-- @param #any ... Arguments to pass to each event handler
-- @return boolean True if no further handlers should be called
function aux_util.callEventHandlers(handlers, ...)
    if handlers then
        for i = #handlers, 1, -1 do
            if handlers[i](...) == false then
                return true
            end
        end
    end
    return false
end

---
-- Iterates over an array of event handler arrays, passing each to `aux_util.callEventHandlers` until the event is handled.
-- @function [parent=#util] callMultipleEventHandlers
-- @param #table handlers An array of event handler arrays
-- @param #any ... Arguments to pass to each event handler
-- @return boolean True if no further handlers should be called
function aux_util.callMultipleEventHandlers(handlers, ...)
    for i = 1, #handlers do
        local stop = aux_util.callEventHandlers(handlers[i], ...)
        if stop then
            return true
        end
    end
    return false
end

return aux_util

