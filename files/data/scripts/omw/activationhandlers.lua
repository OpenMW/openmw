local async = require('openmw.async')
local types = require('openmw.types')
local world = require('openmw.world')

local EnableObject = async:registerTimerCallback('EnableObject', function(obj) obj.enabled = true end)

local function ESM4DoorActivation(door, actor)
    -- TODO: Implement lockpicking minigame
    -- TODO: Play door opening animation and sound
    local Door4 = types.ESM4Door
    if Door4.isTeleport(door) then
        actor:teleport(Door4.destCell(door), Door4.destPosition(door), Door4.destRotation(door))
    else
        door.enabled = false
        async:newSimulationTimer(5, EnableObject, door)
    end
    return false -- disable activation handling in C++ mwmechanics code
end

local function ESM4BookActivation(book, actor)
    if actor.type == types.Player then
        actor:sendEvent('AddUiMode', { mode = 'Book', target = book })
    end
end

local handlersPerObject = {}
local handlersPerType = {}

handlersPerType[types.ESM4Book] = { ESM4BookActivation }
handlersPerType[types.ESM4Door] = { ESM4DoorActivation }

local function onActivate(obj, actor)
    if world.isWorldPaused() then
        return
    end
    if obj.parentContainer then
        return
    end
    local handlers = handlersPerObject[obj.id]
    if handlers then
        for i = #handlers, 1, -1 do
            if handlers[i](obj, actor) == false then
                return -- skip other handlers
            end
        end
    end
    handlers = handlersPerType[obj.type]
    if handlers then
        for i = #handlers, 1, -1 do
            if handlers[i](obj, actor) == false then
                return -- skip other handlers
            end
        end
    end
    types.Actor.activeEffects(actor):remove('invisibility')
    world._runStandardActivationAction(obj, actor)
end

return {
    interfaceName = 'Activation',
    ---
    -- @module Activation
    -- @usage require('openmw.interfaces').Activation
    interface = {
        --- Interface version
        -- @field [parent=#Activation] #number version
        version = 0,

        --- Add new activation handler for a specific object.
        -- If `handler(object, actor)` returns false, other handlers for
        -- the same object (including type handlers) will be skipped.
        -- @function [parent=#Activation] addHandlerForObject
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

        --- Add new activation handler for a type of objects.
        -- If `handler(object, actor)` returns false, other handlers for
        -- the same object (including type handlers) will be skipped.
        -- @function [parent=#Activation] addHandlerForType
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
    engineHandlers = { onActivate = onActivate },
}
