local auxUtil = require('openmw_aux.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local types = {
    Magic = 'Magic',
    Weapon = 'Weapon',
}

local onProjectileHitHandlers = {}

---
-- Table of known projectile types.
-- This contains only 1 entry because only magic projectiles have been given lua handlers so far.
-- @type ProjectileType
-- @field #string Magic

---
-- Table describing a projectile.
-- This is the projectile info provided to projectile hit handlers.
-- @type ProjectileInfo
-- @field #ProjectileType type
-- @field #table userData data that depends on @{#ProjectileType}:
--
--   * `Magic` - userData is a @{SpellCasting#SpellCastInfo}

--- Projectiles interface
-- @module Projectiles
-- @usage require('openmw.interfaces').Projectiles
return {
    engineHandlers = {
        _onProjectileHit = function(projectile, hitResult)
            assert(projectile.type)
            local handlers = onProjectileHitHandlers[projectile.type]
            if handlers then
                auxUtil.callEventHandlers(handlers, projectile, hitResult)
            end
        end,
    },

    interfaceName = 'Projectiles',
    interface = {
        version = 1,

        --- Add new projectile hit handler.
        -- If `handler(projectile, hitResult)` returns false, other handlers for the same projectile
        -- will be skipped. Two handler parameters are a @{#ProjectileInfo} and a @{openmw.nearby#RayCastingResult},
        -- respectively
        -- @function [parent=#Projectiles] addOnProjectileHitHandler
        -- @param #function handler The handler.
        -- @param #string type The type of projectile to handle (see @{#ProjectileInfo.type})
        addOnProjectileHitHandler = function(type, handler)
            assert(type)
            onProjectileHitHandlers[type] = onProjectileHitHandlers[type] or {}
            onProjectileHitHandlers[type][#onProjectileHitHandlers[type]+1] = handler
        end,
        --- @{#ProjectileType}
        -- @field [parent=#Projectiles] #ProjectileType TYPES Available projectile types
        TYPES = types,
    },
}
