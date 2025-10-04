return {
    adjustDamageForArmor = function(damage, actor) return damage end,
    adjustDamageForDifficulty = function(attack, defendant) end,
    applyArmor = function(attack) end,
    getArmorRating = function(actor) return 0 end,
    getArmorSkill = function(item) return nil end,
    getSkillAdjustedArmorRating = function(item, actor) return 0 end,
    getEffectiveArmorRating = function(item, actor) return 0 end,
    spawnBloodEffect = function(position) end,
    pickRandomArmor = function(actor) return nil end
}
