local content = require('openmw.content')
local util = require('openmw.util')

local function asNumber(value)
    if type(value) ~= 'number' then
        return 0
    end
    return value
end

local function generateAttributes()
    local gmsts = content.gameSettings.records
    local attributes = content.attributes.records
    local function getString(key)
        local v = gmsts[key]
        if type(v) ~= 'string' or v == '' then
            return key
        end
        return v
    end

    local values = {
        { id = 'Strength', name = 'sAttributeStrength', description = 'sStrDesc', icon = 'icons/k/attribute_strength.dds', werewolf = 'fWerewolfStrength' },
        { id = 'Intelligence', name = 'sAttributeIntelligence', description = 'sIntDesc', icon = 'icons/k/attribute_int.dds', werewolf = 'fWerewolfIntellegence' },
        { id = 'Willpower', name = 'sAttributeWillpower', description = 'sWilDesc', icon = 'icons/k/attribute_wilpower.dds', werewolf = 'fWerewolfWillpower' },
        { id = 'Agility', name = 'sAttributeAgility', description = 'sAgiDesc', icon = 'icons/k/attribute_agility.dds', werewolf = 'fWerewolfAgility' },
        { id = 'Speed', name = 'sAttributeSpeed', description = 'sSpdDesc', icon = 'icons/k/attribute_speed.dds', werewolf = 'fWerewolfSpeed' },
        { id = 'Endurance', name = 'sAttributeEndurance', description = 'sEndDesc', icon = 'icons/k/attribute_endurance.dds', werewolf = 'fWerewolfEndurance' },
        { id = 'Personality', name = 'sAttributePersonality', description = 'sPerDesc', icon = 'icons/k/attribute_personality.dds', werewolf = 'fWerewolfPersonality' },
        { id = 'Luck', name = 'sAttributeLuck', description = 'sLucDesc', icon = 'icons/k/attribute_luck.dds', werewolf = 'fWerewolfLuck' },
    }
    for _, value in pairs(values) do
        attributes[value.id] = {
            name = getString(value.name),
            description = getString(value.description),
            icon = value.icon,
            werewolfValue = asNumber(gmsts[value.werewolf]),
        }
    end
end

local function setSkillProperties()
    local gmsts = content.gameSettings.records
    local skills = content.skills.records
    local function getString(key)
        local v = gmsts[key]
        if type(v) ~= 'string' or v == '' then
            return key
        end
        return v
    end

    local values = {
        { 'Block', 'sSkillBlock', 'icons/k/combat_block.dds', 'fWerewolfBlock' },
        { 'Armorer', 'sSkillArmorer', 'icons/k/combat_armor.dds', 'fWerewolfArmorer' },
        { 'MediumArmor', 'sSkillMediumarmor', 'icons/k/combat_mediumarmor.dds', 'fWerewolfMediumarmor' },
        { 'HeavyArmor', 'sSkillHeavyarmor', 'icons/k/combat_heavyarmor.dds', 'fWerewolfHeavyarmor' },
        { 'BluntWeapon', 'sSkillBluntweapon', 'icons/k/combat_blunt.dds', 'fWerewolfBluntweapon' },
        { 'LongBlade', 'sSkillLongblade', 'icons/k/combat_longblade.dds', 'fWerewolfLongblade' },
        { 'Axe', 'sSkillAxe', 'icons/k/combat_axe.dds', 'fWerewolfAxe' },
        { 'Spear', 'sSkillSpear', 'icons/k/combat_spear.dds', 'fWerewolfSpear' },
        { 'Athletics', 'sSkillAthletics', 'icons/k/combat_athletics.dds', 'fWerewolfAthletics' },
        { 'Enchant', 'sSkillEnchant', 'icons/k/magic_enchant.dds', 'fWerewolfEnchant' },
        { 'Destruction', 'sSkillDestruction', 'icons/k/magic_destruction.dds', 'fWerewolfDestruction', 'destruction' },
        { 'Alteration', 'sSkillAlteration', 'icons/k/magic_alteration.dds', 'fWerewolfAlteration', 'alteration' },
        { 'Illusion', 'sSkillIllusion', 'icons/k/magic_illusion.dds', 'fWerewolfIllusion', 'illusion' },
        { 'Conjuration', 'sSkillConjuration', 'icons/k/magic_conjuration.dds', 'fWerewolfConjuration', 'conjuration' },
        { 'Mysticism', 'sSkillMysticism', 'icons/k/magic_mysticism.dds', 'fWerewolfMysticism', 'mysticism' },
        { 'Restoration', 'sSkillRestoration', 'icons/k/magic_restoration.dds', 'fWerewolfRestoration', 'restoration' },
        { 'Alchemy', 'sSkillAlchemy', 'icons/k/magic_alchemy.dds', 'fWerewolfAlchemy' },
        { 'Unarmored', 'sSkillUnarmored', 'icons/k/magic_unarmored.dds', 'fWerewolfUnarmored' },
        { 'Security', 'sSkillSecurity', 'icons/k/stealth_security.dds', 'fWerewolfSecurity' },
        { 'Sneak', 'sSkillSneak', 'icons/k/stealth_sneak.dds', 'fWerewolfSneak' },
        { 'Acrobatics', 'sSkillAcrobatics', 'icons/k/stealth_acrobatics.dds', 'fWerewolfAcrobatics' },
        { 'LightArmor', 'sSkillLightarmor', 'icons/k/stealth_lightarmor.dds', 'fWerewolfLightarmor' },
        { 'ShortBlade', 'sSkillShortblade', 'icons/k/stealth_shortblade.dds', 'fWerewolfShortblade' },
        { 'Marksman', 'sSkillMarksman', 'icons/k/stealth_marksman.dds', 'fWerewolfMarksman' },
        { 'Mercantile', 'sSkillMercantile', 'icons/k/stealth_mercantile.dds', 'fWerewolfMerchantile' },
        { 'Speechcraft', 'sSkillSpeechcraft', 'icons/k/stealth_speechcraft.dds', 'fWerewolfSpeechcraft' },
        { 'Handtohand', 'sSkillHandtohand', 'icons/k/stealth_handtohand.dds', 'fWerewolfHandtohand' },
    }
    for _, value in pairs(values) do
        local id, name, icon, werewolf, school = unpack(value)
        local skill = skills[id]
        skill.name = getString(name)
        skill.icon = icon
        skill.werewolfValue = asNumber(gmsts[werewolf])
        if school then
            skill.school = {
                areaSound = school .. ' area',
                boltSound = school .. ' bolt',
                castSound = school .. ' cast',
                failureSound = 'Spell Failure ' .. school,
                hitSound = school .. ' hit',
                name = getString('sSchool' .. school),
                autoCalcMax = asNumber(gmsts['iAutoSpell' .. school .. 'Max']),
            }
        end
    end
end

local function generateDefaultStatics()
    local statics = {
        -- Total conversions from SureAI lack marker records
        divinemarker = 'meshes/marker_divine.nif',
        doormarker = 'meshes/marker_arrow.nif',
        northmarker = 'meshes/marker_north.nif',
        templemarker = 'meshes/marker_temple.nif',
        travelmarker = 'meshes/marker_travel.nif',
    }
    for id, model in pairs(statics) do
        if content.statics.records[id] == nil then
            content.statics.records[id] = { model = model }
        end
    end
end

local function generateDefaultGMSTs()
    local gmsts = {
        -- Companion (tribunal)
        sCompanionShare = 'Companion Share',
        sCompanionWarningMessage = 'Warning message',
        sCompanionWarningButtonOne = 'Button 1',
        sCompanionWarningButtonTwo = 'Button 2',
        sProfitValue = 'Profit Value',
        sTeleportDisabled = 'Teleport disabled',
        sLevitateDisabled = 'Levitate disabled',
        -- Missing in unpatched MW 1.0
        sDifficulty = 'Difficulty',
        fDifficultyMult = 5,
        sAuto_Run = 'Auto Run',
        sServiceRefusal = 'Service Refusal',
        sNeedOneSkill = 'Need one skill',
        sNeedTwoSkills = 'Need two skills',
        sEasy = 'Easy',
        sHard = 'Hard',
        sDeleteNote = 'Delete Note',
        sEditNote = 'Edit Note',
        sAdmireSuccess = 'Admire Success',
        sAdmireFail = 'Admire Fail',
        sIntimidateSuccess = 'Intimidate Success',
        sIntimidateFail = 'Intimidate Fail',
        sTauntSuccess = 'Taunt Success',
        sTauntFail = 'Taunt Fail',
        sBribeSuccess = 'Bribe Success',
        sBribeFail = 'Bribe Fail',
        fNPCHealthBarTime = 5,
        fNPCHealthBarFade = 1,
        fFleeDistance = 3000,
        sMaxSale = 'Max Sale',
        sAnd = 'and',
        -- Werewolf (BM)
        fWereWolfRunMult = 1.3,
        fWereWolfSilverWeaponDamageMult = 2,
        iWerewolfFightMod = 100,
        iWereWolfFleeMod = 100,
        iWereWolfLevelToAttack = 20,
        iWereWolfBounty = 1000,
        fCombatDistanceWerewolfMod = 0.3,
    }
    local store = content.gameSettings.records
    for id, value in pairs(gmsts) do
        if store[id] == nil then
            store[id] = value
        end
    end
    local fallbacks = content.gameSettings.getFallbacks()
    for id, value in pairs(fallbacks) do
        store[id] = value
    end
end

local function generateDefaultDoors()
    local doors = {
        prisonmarker = 'meshes/marker_prison.nif'
    }
    for id, model in pairs(doors) do
        if content.doors.records[id] == nil then
            content.doors.records[id] = { model = model }
        end
    end
end

local function setMagicEffectNames()
    local gmsts = content.gameSettings.records
    local effects = content.magicEffects.records
    for id, gmst in pairs(content.magicEffects._getGMSTs()) do
        local effect = effects[id]
        if effect ~= nil then
            local name = gmsts[gmst]
            if type(name) ~= 'string' or name == '' then
                name = gmst
            end
            effect.name = name
        end
    end
end

local function generateDefaultWeathers()
    local weathers = {
        { "Clear", 1, 0 },
        { "Cloudy", 0.9, 0 },
        { "Foggy", 0.2, 30 },
        { "Overcast", 0.7, 0 },
        { "Rain", 0.5, 10 },
        { "Thunderstorm", 0.5, 20 },
        { "Ashstorm", 0.2, 50, 'meshes/ashcloud.nif' },
        { "Blight", 0.2, 60, 'meshes/blightcloud.nif' },
        { "Snow", 0.5, 40, 'meshes/snow.nif' },
        { "Blizzard", 0.16, 70, 'meshes/blizzard.nif' },
    }
    local gmsts = content.gameSettings.records
    local commaString = util.color.commaString
    local function getColor(id)
        local ok, c = pcall(function() return commaString(gmsts[id]) end)
        if not ok then
            print('Setting', id, 'is not a valid color, using middle gray as a fallback')
            return util.color.rgba(0.5, 0.5, 0.5, 1)
        end
        return c
    end
    local function getFloat(id)
        return asNumber(gmsts[id])
    end
    local function getString(id, fallback)
        local v = gmsts[id]
        if type(v) ~= 'string' or v == '' then
            return fallback or ''
        end
        return v
    end
    local function getBool(id)
        return asNumber(gmsts[id]) ~= 0
    end
    local function getInterpolatorColor(name, color, time)
        return getColor(('Weather_%s_%s_%s_Color'):format(name, color, time))
    end
    local function getColorInterpolator(name, color)
        return {
            sunrise = getInterpolatorColor(name, color, 'Sunrise'),
            day = getInterpolatorColor(name, color, 'Day'),
            sunset = getInterpolatorColor(name, color, 'Sunset'),
            night = getInterpolatorColor(name, color, 'Night'),
        }
    end
    local rainSpeed = getFloat('Weather_Precip_Gravity')
    local rainRipples = getBool('Weather_Rain_Ripples')
    local snowRipples = getBool('Weather_Snow_Ripples')
    local records = content.weathers.records
    for index, table in ipairs(weathers) do
        local recordId, dlFactor, dlOffset, particleEffect = unpack(table)
        local fogDepthDay = getFloat(('Weather_%s_Land_Fog_Day_Depth'):format(recordId))
        local fogDepthNight = getFloat(('Weather_%s_Land_Fog_Night_Depth'):format(recordId))
        local windSpeed = getFloat(('Weather_%s_Wind_Speed'):format(recordId))
        local fStromWindSpeed = getFloat('fStromWindSpeed')
        local rainEntranceSpeed = getFloat(('Weather_%s_Rain_Entrance_Speed'):format(recordId))
        local weather = {
            scriptId = index - 1,
            name = recordId,
            particleEffect = particleEffect,
            skyColor = getColorInterpolator(recordId, 'Sky'),
            fogColor = getColorInterpolator(recordId, 'Fog'),
            ambientColor = getColorInterpolator(recordId, 'Ambient'),
            sunColor = getColorInterpolator(recordId, 'Sun'),
            landFogDepth = {
                sunrise = fogDepthDay,
                day = fogDepthDay,
                sunset = fogDepthDay,
                night = fogDepthNight,
            },
            sunDiscSunsetColor = getColor(('Weather_%s_Sun_Disc_Sunset_Color'):format(recordId)),
            windSpeed = windSpeed,
            cloudSpeed = getFloat(('Weather_%s_Cloud_Speed'):format(recordId)),
            glareView = getFloat(('Weather_%s_Glare_View'):format(recordId)),
            rainSpeed = rainSpeed,
            rainEntranceSpeed = rainEntranceSpeed > 0 and rainEntranceSpeed or nil,
            rainMaxRaindrops = getFloat(('Weather_%s_Max_Raindrops'):format(recordId)),
            rainDiameter = getFloat(('Weather_%s_Rain_Diameter'):format(recordId)),
            rainThreshold = getFloat(('Weather_%s_Rain_Threshold'):format(recordId)),
            rainMinHeight = getFloat(('Weather_%s_Rain_Height_Min'):format(recordId)),
            rainMaxHeight = getFloat(('Weather_%s_Rain_Height_Max'):format(recordId)),
            cloudsMaximumPercent = getFloat(('Weather_%s_Clouds_Maximum_Percent'):format(recordId)),
            isStorm = windSpeed > fStromWindSpeed,
            cloudTexture = getString(('Weather_%s_Cloud_Texture'):format(recordId)),
            distantLandFogFactor = dlFactor,
            distantLandFogOffset = dlOffset,
            thunderSoundID = {
                getString(('Weather_%s_Thunder_Sound_ID_0'):format(recordId)),
                getString(('Weather_%s_Thunder_Sound_ID_1'):format(recordId)),
                getString(('Weather_%s_Thunder_Sound_ID_2'):format(recordId)),
                getString(('Weather_%s_Thunder_Sound_ID_3'):format(recordId)),
            },
            ambientLoopSoundID = getString(('Weather_%s_Ambient_Loop_Sound_ID'):format(recordId)),
            transitionDelta = getFloat(('Weather_%s_Transition_Delta'):format(recordId)),
            thunderFrequency = getFloat(('Weather_%s_Thunder_Frequency'):format(recordId)),
            thunderThreshold = getFloat(('Weather_%s_Thunder_Threshold'):format(recordId)),
            flashDecrement = getFloat(('Weather_%s_Flash_Decrement'):format(recordId)),
        }
        if weather.ambientLoopSoundID == 'None' then
            weather.ambientLoopSoundID = nil
        end
        if getBool(('Weather_%s_Using_Precip'):format(recordId)) then
            weather.rainEffect = 'meshes/raindrop.nif'
            weather.rainLoopSoundID = getString(('Weather_%s_Rain_Loop_Sound_ID'):format(recordId), 'rain')
            if weather.rainLoopSoundID == 'None' then
                weather.rainLoopSoundID = nil
            end
            weather.hasRipples = rainRipples
            weather.isOccluded = true
        elseif recordId == 'Snow' then
            weather.hasRipples = snowRipples
            weather.isOccluded = true
        end
        -- Morrowind deliberately rotates the blizzard mesh, so so should we.
        if recordId == 'Blizzard' then
            weather.stormParticleRotation = util.vector3(-1, 0, 0)
        end
        weather.canUseTorches = (not weather.rainEffect and not particleEffect) or recordId == 'Ashstorm'
        records[recordId] = weather
    end
end

return {
    engineHandlers = {
        onContentFilesLoaded = function()
            generateAttributes()
            setSkillProperties()
            generateDefaultDoors()
            generateDefaultGMSTs()
            generateDefaultWeathers()
            generateDefaultStatics()
            setMagicEffectNames()
        end
    }
}
