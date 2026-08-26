local util = require('openmw.util')
local I = require('openmw.interfaces')

return {
    interfaceName = 'SpellCasting',
    interface = {
        version = 0,
        explodeSpell = function(spellcast, options) end,
        inflict = function(spellcast, target, range) end,
    },
}
