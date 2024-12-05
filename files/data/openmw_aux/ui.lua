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
        elseif type(v) == 'table' and getmetatable(v) == nil then
            result[k] = aux_ui.deepLayoutCopy(v)
        else
            result[k] = v
        end
    end
    return result
end

local function isUiElement(v)
    return v.__type and v.__type.name == 'LuaUi::Element'
end

local function deepElementCallback(layout, callback)
    if not layout.content then return end
    for i = 1, #layout.content do
        local child = layout.content[i]
        if isUiElement(child) then
            callback(child)
            deepElementCallback(child.layout, callback)
        else
            deepElementCallback(child, callback)
        end
    end
end

---
-- Recursively updates all elements in the passed layout or element
-- @function [parent=#ui] deepUpdate
-- @param #any elementOrLayout  @{openmw.ui#Layout} or  @{openmw.ui#Element}
function aux_ui.deepUpdate(elementOrLayout)
    local layout = elementOrLayout
    if elementOrLayout.update then
        elementOrLayout:update()
        layout = elementOrLayout.layout
    end
    deepElementCallback(layout, function (e) e:update() end)
end

---
-- Recursively destroys all elements in the passed layout or element
-- @function [parent=#ui] deepDestroy
-- @param #any elementOrLayout  @{openmw.ui#Layout} or  @{openmw.ui#Element}
function aux_ui.deepDestroy(elementOrLayout)
    local layout = elementOrLayout
    if elementOrLayout.destroy then
        elementOrLayout:destroy()
        layout = elementOrLayout.layout
    end
    deepElementCallback(layout, function (e) e:destroy() end)
end

return aux_ui
