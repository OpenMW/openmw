local util = require('openmw.util')

return {
    interfaceName = 'Combat',
    interface = {
        version = 4,
        adjustDamageForArmor = function(damage, actor) return damage end,
        adjustDamageForDifficulty = function(attack, defendant) end,
        getArmorRating = function(actor) return 0 end,
        getArmorSkill = function(itemOrId) return nil end,
        getSkillAdjustedArmorRating = function(itemOrId, actor) return 0 end,
        getEffectiveArmorRating = function(item, actor) return 0 end,
        pickRandomArmor = function(actor) return nil end,

        -- duplicating these tables from interface_local.lua rather than having to
        -- make a common.lua just for these
        ATTACK_SOURCE_TYPES = util.makeStrictReadOnly({
            Magic = 'magic',
            Melee = 'melee',
            Ranged = 'ranged',
            Unspecified = 'unspecified',
        }),
        ATTACK_TYPES = util.makeStrictReadOnly({
            Chop = 0,
            Slash = 1,
            Thrust = 2,
        }),
    },
}
