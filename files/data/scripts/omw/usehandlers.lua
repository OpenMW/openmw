local types = require('openmw.types')
local world = require('openmw.world')
local auxUtil = require('openmw_aux.util')

local handlersPerObject = {}
local handlersPerType = {}

local function useItem(obj, actor, force)
    local options = { force = force or false }
    local handled = auxUtil.callMultipleEventHandlers({ handlersPerObject[obj.id], handlersPerType[obj.type] }, obj, actor, options)
    if handled then
        return
    end
    world._runStandardUseAction(obj, actor, options.force)
end

return {
    interfaceName = 'ItemUsage',
    ---
    -- Allows to extend or override built-in item usage mechanics.
    -- Note: at the moment it can override item usage in inventory
    -- (dragging an item on the character's model), but
    --
    -- * can't intercept actions performed by mwscripts;
    -- * can't intercept actions performed by the AI (i.e. drinking a potion in combat);
    -- * can't intercept actions performed via quick keys menu.
    -- @module ItemUsage
    -- @context global
    -- @usage local I = require('openmw.interfaces')
    --
    -- -- Override Use action (global script).
    -- -- Forbid equipping armor with weight > 5
    -- I.ItemUsage.addHandlerForType(types.Armor, function(armor, actor)
    --     if types.Armor.record(armor).weight > 5 then
    --         return false -- disable other handlers
    --     end
    -- end)
    --
    -- -- Call Use action (any script).
    -- core.sendGlobalEvent('UseItem', {object = armor, actor = player})
    interface = {
        --- Interface version
        -- @field [parent=#ItemUsage] #number version
        version = 0,

        --- Add new use action handler for a specific object.
        -- If `handler(object, actor, options)` returns false, other handlers for
        -- the same object (including type handlers) will be skipped.
        -- @function [parent=#ItemUsage] addHandlerForObject
        -- @param openmw.core#GameObject obj The object.
        -- @param #function handler The handler.
        addHandlerForObject = function(obj, handler)
            local handlers = handlersPerObject[obj.id]
            if handlers == nil then
                handlers = {}
                handlersPerObject[obj.id] = handlers
            end
            handlers[#handlers + 1] = handler
        end,

        --- Add new use action handler for a type of objects.
        -- If `handler(object, actor, options)` returns false, other handlers for
        -- the same object (including type handlers) will be skipped.
        -- @function [parent=#ItemUsage] addHandlerForType
        -- @param #any type A type from the `openmw.types` package.
        -- @param #function handler The handler.
        addHandlerForType = function(type, handler)
            local handlers = handlersPerType[type]
            if handlers == nil then
                handlers = {}
                handlersPerType[type] = handlers
            end
            handlers[#handlers + 1] = handler
        end,
    },
    engineHandlers = { _onUseItem = useItem },
    eventHandlers = {
        UseItem = function(data)
            if not data.object then
                error('UseItem: missing argument "object"')
            end
            if not data.actor or not types.Actor.objectIsInstance(data.actor) then
                error('UseItem: invalid argument "actor"')
            end
            useItem(data.object, data.actor, data.force)
        end
    }
}
