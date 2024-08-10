local anim = require('openmw.animation')
local self = require('openmw.self')

local playBlendedHandlers = {}
local function onPlayBlendedAnimation(groupname, options)    
    for i = #playBlendedHandlers, 1, -1 do
        if playBlendedHandlers[i](groupname, options) == false then
            return
        end
    end
end

local function playBlendedAnimation(groupname, options)
    onPlayBlendedAnimation(groupname, options)
    if options.skip then
        return
    end
    anim.playBlended(self, groupname, options)
end

local textKeyHandlers = {}
local function onAnimationTextKey(groupname, key)
    local handlers = textKeyHandlers[groupname]
    if handlers then
        for i = #handlers, 1, -1 do
            if handlers[i](groupname, key) == false then
                return
            end
        end
    end
    handlers = textKeyHandlers['']
    if handlers then
        for i = #handlers, 1, -1 do
            if handlers[i](groupname, key) == false then
                return
            end
        end
    end
end

local initialized = false

local function onUpdate(dt)
    -- The script is loaded before the actor's CharacterController object is initialized, therefore
    -- we have to delay this initialization step or the call won't have any effect.
    if not initialized then
        self:_enableLuaAnimations(true)
        initialized = true
    end
end

return {
    engineHandlers = { 
        _onPlayAnimation = playBlendedAnimation,
        _onAnimationTextKey = onAnimationTextKey,
        onUpdate = onUpdate,
    },
    
    interfaceName = 'AnimationController',
    --- 
    -- Animation controller interface
    -- @module AnimationController
    -- @usage local anim = require('openmw.animation')
    -- local I = require('openmw.interfaces')
    --
    -- -- play spellcast animation 
    -- I.AnimationController.playBlendedAnimation('spellcast', { startkey = 'self start', stopkey = 'self stop', priority = {
    --      [anim.BONE_GROUP.RightArm] = anim.PRIORITY.Weapon,
    --      [anim.BONE_GROUP.LeftArm] = anim.PRIORITY.Weapon,
    --      [anim.BONE_GROUP.Torso] = anim.PRIORITY.Weapon,
    --      [anim.BONE_GROUP.LowerBody] = anim.PRIORITY.WeaponLowerBody
    --      } })
    -- 
    -- @usage -- react to the spellcast release textkey
    -- I.AnimationController.addTextKeyHandler('spellcast', function(groupname, key)
    --     -- Note, Lua is 1-indexed so have to subtract 1 less than the length of 'release'
    --     if key.sub(key, #key - 6) == 'release' then
    --         print('Abra kadabra!')
    --     end
    -- end)
    --
    -- @usage -- Add a text key handler that will react to all keys
    -- I.AnimationController.addTextKeyHandler('', function(groupname, key)
    --     if key.sub(key, #key - 2) == 'hit' and not key.sub(key, #key - 7) == ' min hit' then
    --         print('Hit!')
    --     end
    -- end)
    -- 
    -- @usage -- Make a handler that changes player attack speed based on current fatigue
    -- I.AnimationController.addPlayBlendedAnimationHandler(function (groupname, options)
    --     local stop = options.stopkey
    --     if #stop > 10 and stop.sub(stop, #stop - 10) == ' max attack' then
    --         -- This is an attack wind up animation, scale its speed by attack 
    --         local fatigue = Actor.stats.dynamic.fatigue(self)
    --         local factor = 1 - fatigue.current / fatigue.base
    --         speed = 1 - factor * 0.8
    --         options.speed = speed
    --     end
    -- end)
    -- 
    
    interface = {
        --- Interface version
        -- @field [parent=#AnimationController] #number version
        version = 0,
        
        --- AnimationController Package
        -- @type Package
        
        --- Make this actor play an animation. Makes a call to @{openmw.animation#playBlended}, after invoking handlers added through addPlayBlendedAnimationHandler
        -- @function [parent=#AnimationController] playBlendedAnimation
        -- @param #string groupname The animation group to be played
        -- @param #table options The table of play options that will be passed to @{openmw.animation#playBlended}
        playBlendedAnimation = playBlendedAnimation,

        --- Add new playBlendedAnimation handler for this actor
        -- If `handler(groupname, options)` returns false, other handlers for
        -- the call will be skipped.
        -- @function [parent=#AnimationController] addPlayBlendedAnimationHandler
        -- @param #function handler The handler.
        addPlayBlendedAnimationHandler = function(handler)
            playBlendedHandlers[#playBlendedHandlers + 1] = handler
        end,

        --- Add new text key handler for this actor
        -- While playing, some animations emit text key events. Register a handle to listen for all
        -- text key events associated with this actor's animations.
        -- If `handler(groupname, key)` returns false, other handlers for
        -- the call will be skipped.
        -- @function [parent=#AnimationController] addTextKeyHandler
        -- @param #string groupname Name of the animation group to listen to keys for. If the empty string or nil, all keys will be received
        -- @param #function handler The handler.
        addTextKeyHandler = function(groupname, handler)
            if not groupname then
                groupname = ""
            end
            local handlers = textKeyHandlers[groupname]
            if handlers == nil then
                handlers = {}
                textKeyHandlers[groupname] = handlers
            end
            handlers[#handlers + 1] = handler
        end,
    },

    eventHandlers = {
        AddVfx = function(data)
            anim.addVfx(self, data.model, data.options)
        end,
    }
}
