local self = require('openmw.self')
local core = require('openmw.core')
local types = require('openmw.types')
local Actor = types.Actor

return {
    eventHandlers = {
        ModifyStat = function(data)
            local stat = Actor.stats.dynamic[data.stat](self)
            stat.current = stat.current + data.amount
        end,
        PlaySound3d = function(data)
            if data.sound then
                core.sound.playSound3d(data.sound, self, data.options)
            else
                core.sound.playSoundFile3d(data.file, self, data.options)
            end
        end,
        BreakInvisibility = function(data)
            Actor.activeEffects(self):remove(core.magic.EFFECT_TYPE.Invisibility)
        end,
        Unequip = function(data)
            local equipment = Actor.getEquipment(self)
            if data.item then
                for slot, item in pairs(equipment) do
                    if item == data.item then
                        equipment[slot] = nil
                    end
                end
            elseif data.slot then
                equipment[slot] = nil
            end
            Actor.setEquipment(self, equipment)
        end,
    },
}
