local util = require('openmw.util')

local function shallowLayoutCopy(source, target)
    for k in pairs(target) do
        target[k] = nil
    end
    for k, v in pairs(source) do
        target[k] = v
    end
    return target
end

---
-- @type Templates
-- @usage
-- local I = require('openmw.interfaces')
-- local ui = require('openmw.ui')
-- local auxUi = require('openmw_aux.ui')
-- ui.create {
--     template = I.MWUI.templates.textNormal,
--     layer = 'Windows',
--     type = ui.TYPE.Text,
--     props = {
--         text = 'Hello, world!',
--     },
-- }
-- -- important to copy here
-- local myText = auxUi.deepLayoutCopy(I.MWUI.templates.textNormal)
-- myText.props.textSize = 20
-- I.MWUI.templates.textNormal = myText
-- ui.updateAll()

local templatesMeta = {
    __index = function(self, key)
        return self.__templates[key]
    end,
    __newindex = function(self, key, template)
        local target = self.__templates[key]
        if target == template then
            error("Overriding a template with itself")
        else
            shallowLayoutCopy(template, target)
        end
    end,
}

---
-- @module MWUI
-- @usage require('openmw.interfaces').MWUI
local function TemplateOverrides(templates)
    return setmetatable({
        __templates = util.makeReadOnly(templates),
    }, templatesMeta)
end

---
-- @field [parent=#MWUI] #Templates templates
local templates = {}

---
-- Container that adds padding around its content.
-- @field [parent=#MWUI] #table padding

---
-- Standard spacing interval
-- @field [parent=#MWUI] #number interval
require('scripts.omw.mwui.space')(templates)

---
-- Standard rectangular border
-- @field [parent=#Templates] openmw.ui#Layout border

---
-- Container wrapping the content with borders
-- @field [parent=#Templates] openmw.ui#Layout box

---
-- Same as box, but with a semi-transparent background
-- @field [parent=#Templates] openmw.ui#Layout boxTransparent
---
-- Same as box, but with a solid background
-- @field [parent=#Templates] openmw.ui#Layout boxSolid
require('scripts.omw.mwui.borders')(templates)

---
-- Standard "sand" colored text
-- @field [parent=#Templates] openmw.ui#Layout textNormal
require('scripts.omw.mwui.text')(templates)

---
-- Single line text input
-- @field [parent=#Templates] openmw.ui#Layout textEditLine

---
-- Multiline text input
-- @field [parent=#Templates] openmw.ui#Layout textEditBox
require('scripts.omw.mwui.textEdit')(templates)

---
-- Shades its children and makes them uninteractible
-- @field [parent=#Templates] openmw.ui#Layout disabled
require('scripts.omw.mwui.filters')(templates)

---
-- Interface version
-- @field [parent=#MWUI] #number version
local interface = {
    version = 0,
    templates = TemplateOverrides(templates),
}

return {
    interfaceName = "MWUI",
    interface = interface,
}