local self = require("openmw.self")
local core = require("openmw.core")
local types = require("openmw.types")



return {
    engineHandlers = {
        onTeleported = function ()
            core.sendGlobalEvent("builtin:map_extractor:teleport", self)
        end
    },
    eventHandlers = {
        ["builtin:map_extractor:updateMenu"] = function (data)
            types.Player.sendMenuEvent(self, "builtin:map_extractor:updateMenu", data)
        end,
    }
}