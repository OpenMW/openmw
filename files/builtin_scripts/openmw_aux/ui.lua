local ui = require('openmw.ui')

---
-- `openmw_aux.ui` defines utility functions for UI.
-- Implementation can be found in `resources/vfs/openmw_aux/ui.lua`.
-- @module ui
-- @usage local auxUi = require('openmw_aux.ui')
local aux_ui = {}

local function deepContentCopy(content)
    local result = ui.content{}
    for _, v in ipairs(content) do
        result:add(aux_ui.deepLayoutCopy(v))
    end
    return result
end

---
-- @function [parent=#ui] deepLayoutCopy
-- @param #table layout
-- @return #table copied layout
function aux_ui.deepLayoutCopy(layout)
    local result = {}
    for k, v in pairs(layout) do
        if k == 'content' then
            result[k] = deepContentCopy(v)
        elseif type(v) == 'table' then
            result[k] = aux_ui.deepLayoutCopy(v)
        else
            result[k] = v
        end
    end
    return result
end

return aux_ui
