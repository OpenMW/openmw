#include "labels.hpp"

#include <format>

#include <components/esm3/dialoguecondition.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadweap.hpp>

std::string_view bodyPartLabel(int idx)
{
    if (idx >= 0 && idx <= 26)
    {
        static constexpr std::string_view bodyPartLabels[] = {
            "Head",
            "Hair",
            "Neck",
            "Cuirass",
            "Groin",
            "Skirt",
            "Right Hand",
            "Left Hand",
            "Right Wrist",
            "Left Wrist",
            "Shield",
            "Right Forearm",
            "Left Forearm",
            "Right Upperarm",
            "Left Upperarm",
            "Right Foot",
            "Left Foot",
            "Right Ankle",
            "Left Ankle",
            "Right Knee",
            "Left Knee",
            "Right Leg",
            "Left Leg",
            "Right Shoulder",
            "Left Shoulder",
            "Weapon",
            "Tail",
        };
        return bodyPartLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view meshPartLabel(int idx)
{
    if (idx >= 0 && idx <= ESM::BodyPart::MP_Tail)
    {
        static constexpr std::string_view meshPartLabels[] = {
            "Head",
            "Hair",
            "Neck",
            "Chest",
            "Groin",
            "Hand",
            "Wrist",
            "Forearm",
            "Upperarm",
            "Foot",
            "Ankle",
            "Knee",
            "Upper Leg",
            "Clavicle",
            "Tail",
        };
        return meshPartLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view meshTypeLabel(int idx)
{
    if (idx >= 0 && idx <= ESM::BodyPart::MT_Armor)
    {
        static constexpr std::string_view meshTypeLabels[] = {
            "Skin",
            "Clothing",
            "Armor",
        };
        return meshTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view clothingTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 9)
    {
        static constexpr std::string_view clothingTypeLabels[] = {
            "Pants",
            "Shoes",
            "Shirt",
            "Belt",
            "Robe",
            "Right Glove",
            "Left Glove",
            "Skirt",
            "Ring",
            "Amulet",
        };
        return clothingTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view armorTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 10)
    {
        static constexpr std::string_view armorTypeLabels[] = {
            "Helmet",
            "Cuirass",
            "Left Pauldron",
            "Right Pauldron",
            "Greaves",
            "Boots",
            "Left Gauntlet",
            "Right Gauntlet",
            "Shield",
            "Left Bracer",
            "Right Bracer",
        };
        return armorTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view dialogTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 4)
    {
        static constexpr std::string_view dialogTypeLabels[] = {
            "Topic",
            "Voice",
            "Greeting",
            "Persuasion",
            "Journal",
        };
        return dialogTypeLabels[idx];
    }
    else if (idx == -1)
        return "Deleted";
    else
        return "Invalid";
}

std::string_view questStatusLabel(int idx)
{
    if (idx >= 0 && idx <= 4)
    {
        static constexpr std::string_view questStatusLabels[] = {
            "None",
            "Name",
            "Finished",
            "Restart",
            "Deleted",
        };
        return questStatusLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view creatureTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 3)
    {
        static constexpr std::string_view creatureTypeLabels[] = {
            "Creature",
            "Daedra",
            "Undead",
            "Humanoid",
        };
        return creatureTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view soundTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 7)
    {
        static constexpr std::string_view soundTypeLabels[] = {
            "Left Foot",
            "Right Foot",
            "Swim Left",
            "Swim Right",
            "Moan",
            "Roar",
            "Scream",
            "Land",
        };
        return soundTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view weaponTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 13)
    {
        static constexpr std::string_view weaponTypeLabels[] = {
            "Short Blade One Hand",
            "Long Blade One Hand",
            "Long Blade Two Hand",
            "Blunt One Hand",
            "Blunt Two Close",
            "Blunt Two Wide",
            "Spear Two Wide",
            "Axe One Hand",
            "Axe Two Hand",
            "Marksman Bow",
            "Marksman Crossbow",
            "Marksman Thrown",
            "Arrow",
            "Bolt",
        };
        return weaponTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view aiTypeLabel(ESM::AiPackageType type)
{
    switch (type)
    {
        case ESM::AI_Wander:
            return "Wander";
        case ESM::AI_Travel:
            return "Travel";
        case ESM::AI_Follow:
            return "Follow";
        case ESM::AI_Escort:
            return "Escort";
        case ESM::AI_Activate:
            return "Activate";
    }
    return "Invalid";
}

std::string_view magicEffectLabel(int idx)
{
    if (idx >= 0 && idx <= 142)
    {
        static constexpr std::string_view magicEffectLabels[] = {
            "Water Breathing",
            "Swift Swim",
            "Water Walking",
            "Shield",
            "Fire Shield",
            "Lightning Shield",
            "Frost Shield",
            "Burden",
            "Feather",
            "Jump",
            "Levitate",
            "SlowFall",
            "Lock",
            "Open",
            "Fire Damage",
            "Shock Damage",
            "Frost Damage",
            "Drain Attribute",
            "Drain Health",
            "Drain Magicka",
            "Drain Fatigue",
            "Drain Skill",
            "Damage Attribute",
            "Damage Health",
            "Damage Magicka",
            "Damage Fatigue",
            "Damage Skill",
            "Poison",
            "Weakness to Fire",
            "Weakness to Frost",
            "Weakness to Shock",
            "Weakness to Magicka",
            "Weakness to Common Disease",
            "Weakness to Blight Disease",
            "Weakness to Corprus Disease",
            "Weakness to Poison",
            "Weakness to Normal Weapons",
            "Disintegrate Weapon",
            "Disintegrate Armor",
            "Invisibility",
            "Chameleon",
            "Light",
            "Sanctuary",
            "Night Eye",
            "Charm",
            "Paralyze",
            "Silence",
            "Blind",
            "Sound",
            "Calm Humanoid",
            "Calm Creature",
            "Frenzy Humanoid",
            "Frenzy Creature",
            "Demoralize Humanoid",
            "Demoralize Creature",
            "Rally Humanoid",
            "Rally Creature",
            "Dispel",
            "Soultrap",
            "Telekinesis",
            "Mark",
            "Recall",
            "Divine Intervention",
            "Almsivi Intervention",
            "Detect Animal",
            "Detect Enchantment",
            "Detect Key",
            "Spell Absorption",
            "Reflect",
            "Cure Common Disease",
            "Cure Blight Disease",
            "Cure Corprus Disease",
            "Cure Poison",
            "Cure Paralyzation",
            "Restore Attribute",
            "Restore Health",
            "Restore Magicka",
            "Restore Fatigue",
            "Restore Skill",
            "Fortify Attribute",
            "Fortify Health",
            "Fortify Magicka",
            "Fortify Fatigue",
            "Fortify Skill",
            "Fortify Maximum Magicka",
            "Absorb Attribute",
            "Absorb Health",
            "Absorb Magicka",
            "Absorb Fatigue",
            "Absorb Skill",
            "Resist Fire",
            "Resist Frost",
            "Resist Shock",
            "Resist Magicka",
            "Resist Common Disease",
            "Resist Blight Disease",
            "Resist Corprus Disease",
            "Resist Poison",
            "Resist Normal Weapons",
            "Resist Paralysis",
            "Remove Curse",
            "Turn Undead",
            "Summon Scamp",
            "Summon Clannfear",
            "Summon Daedroth",
            "Summon Dremora",
            "Summon Ancestral Ghost",
            "Summon Skeletal Minion",
            "Summon Bonewalker",
            "Summon Greater Bonewalker",
            "Summon Bonelord",
            "Summon Winged Twilight",
            "Summon Hunger",
            "Summon Golden Saint",
            "Summon Flame Atronach",
            "Summon Frost Atronach",
            "Summon Storm Atronach",
            "Fortify Attack",
            "Command Creature",
            "Command Humanoid",
            "Bound Dagger",
            "Bound Longsword",
            "Bound Mace",
            "Bound Battle Axe",
            "Bound Spear",
            "Bound Longbow",
            "EXTRA SPELL",
            "Bound Cuirass",
            "Bound Helm",
            "Bound Boots",
            "Bound Shield",
            "Bound Gloves",
            "Corprus",
            "Vampirism",
            "Summon Centurion Sphere",
            "Sun Damage",
            "Stunted Magicka",
            "Summon Fabricant",
            "sEffectSummonCreature01",
            "sEffectSummonCreature02",
            "sEffectSummonCreature03",
            "sEffectSummonCreature04",
            "sEffectSummonCreature05",
        };
        return magicEffectLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view attributeLabel(int idx)
{
    if (idx >= 0 && idx <= 7)
    {
        static constexpr std::string_view attributeLabels[] = {
            "Strength",
            "Intelligence",
            "Willpower",
            "Agility",
            "Speed",
            "Endurance",
            "Personality",
            "Luck",
        };
        return attributeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view spellTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 5)
    {
        static constexpr std::string_view spellTypeLabels[] = {
            "Spells",
            "Abilities",
            "Blight Disease",
            "Disease",
            "Curse",
            "Powers",
        };
        return spellTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view specializationLabel(int idx)
{
    if (idx >= 0 && idx <= 2)
    {
        static constexpr std::string_view specializationLabels[] = {
            "Combat",
            "Magic",
            "Stealth",
        };
        return specializationLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view skillLabel(int idx)
{
    if (idx >= 0 && idx <= 26)
    {
        static constexpr std::string_view skillLabels[] = {
            "Block",
            "Armorer",
            "Medium Armor",
            "Heavy Armor",
            "Blunt Weapon",
            "Long Blade",
            "Axe",
            "Spear",
            "Athletics",
            "Enchant",
            "Destruction",
            "Alteration",
            "Illusion",
            "Conjuration",
            "Mysticism",
            "Restoration",
            "Alchemy",
            "Unarmored",
            "Security",
            "Sneak",
            "Acrobatics",
            "Light Armor",
            "Short Blade",
            "Marksman",
            "Mercantile",
            "Speechcraft",
            "Hand-to-hand",
        };
        return skillLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view apparatusTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 3)
    {
        static constexpr std::string_view apparatusTypeLabels[] = {
            "Mortar",
            "Alembic",
            "Calcinator",
            "Retort",
        };
        return apparatusTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view rangeTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 2)
    {
        static constexpr std::string_view rangeTypeLabels[] = {
            "Self",
            "Touch",
            "Target",
        };
        return rangeTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view schoolLabel(int idx)
{
    if (idx >= 0 && idx <= 5)
    {
        static constexpr std::string_view schoolLabels[] = {
            "Alteration",
            "Conjuration",
            "Destruction",
            "Illusion",
            "Mysticism",
            "Restoration",
        };
        return schoolLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view enchantTypeLabel(int idx)
{
    if (idx >= 0 && idx <= 3)
    {
        static constexpr std::string_view enchantTypeLabels[] = {
            "Cast Once",
            "Cast When Strikes",
            "Cast When Used",
            "Constant Effect",
        };
        return enchantTypeLabels[idx];
    }
    else
        return "Invalid";
}

std::string_view ruleFunction(int idx)
{
    if (idx >= ESM::DialogueCondition::Function_FacReactionLowest
        && idx <= ESM::DialogueCondition::Function_PcWerewolfKills)
    {
        static constexpr std::string_view ruleFunctions[] = {
            "Lowest Faction Reaction",
            "Highest Faction Reaction",
            "Rank Requirement",
            "NPC Reputation",
            "Health Percent",
            "Player Reputation",
            "NPC Level",
            "Player Health Percent",
            "Player Magicka",
            "Player Fatigue",
            "Player Attribute Strength",
            "Player Skill Block",
            "Player Skill Armorer",
            "Player Skill Medium Armor",
            "Player Skill Heavy Armor",
            "Player Skill Blunt Weapon",
            "Player Skill Long Blade",
            "Player Skill Axe",
            "Player Skill Spear",
            "Player Skill Athletics",
            "Player Skill Enchant",
            "Player Skill Destruction",
            "Player Skill Alteration",
            "Player Skill Illusion",
            "Player Skill Conjuration",
            "Player Skill Mysticism",
            "Player SKill Restoration",
            "Player Skill Alchemy",
            "Player Skill Unarmored",
            "Player Skill Security",
            "Player Skill Sneak",
            "Player Skill Acrobatics",
            "Player Skill Light Armor",
            "Player Skill Short Blade",
            "Player Skill Marksman",
            "Player Skill Mercantile",
            "Player Skill Speechcraft",
            "Player Skill Hand to Hand",
            "Player Gender",
            "Player Expelled from Faction",
            "Player Diseased (Common)",
            "Player Diseased (Blight)",
            "Player Clothing Modifier",
            "Player Crime Level",
            "Player Same Sex",
            "Player Same Race",
            "Player Same Faction",
            "Faction Rank Difference",
            "Player Detected",
            "Alarmed",
            "Choice Selected",
            "Player Attribute Intelligence",
            "Player Attribute Willpower",
            "Player Attribute Agility",
            "Player Attribute Speed",
            "Player Attribute Endurance",
            "Player Attribute Personality",
            "Player Attribute Luck",
            "Player Diseased (Corprus)",
            "Weather",
            "Player is a Vampire",
            "Player Level",
            "Attacked",
            "NPC Talked to Player",
            "Player Health",
            "Creature Target",
            "Friend Hit",
            "Fight",
            "Hello",
            "Alarm",
            "Flee",
            "Should Attack",
            "Werewolf",
            "Werewolf Kills",
        };
        return ruleFunctions[idx];
    }
    else
        return "Invalid";
}

// The "unused flag bits" should probably be defined alongside the
// defined bits in the ESM component.  The names of the flag bits are
// very inconsistent.

std::string bodyPartFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::BodyPart::BPF_Female)
        properties += "Female ";
    if (flags & ESM::BodyPart::BPF_NotPlayable)
        properties += "NotPlayable ";
    int unused = (0xFFFFFFFF ^ (ESM::BodyPart::BPF_Female | ESM::BodyPart::BPF_NotPlayable));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string cellFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Cell::HasWater)
        properties += "HasWater ";
    if (flags & ESM::Cell::Interior)
        properties += "Interior ";
    if (flags & ESM::Cell::NoSleep)
        properties += "NoSleep ";
    if (flags & ESM::Cell::QuasiEx)
        properties += "QuasiEx ";
    // This used value is not in the ESM component.
    if (flags & 0x00000040)
        properties += "Unknown ";
    int unused = (0xFFFFFFFF
        ^ (ESM::Cell::HasWater | ESM::Cell::Interior | ESM::Cell::NoSleep | ESM::Cell::QuasiEx | 0x00000040));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string containerFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Container::Unknown)
        properties += "Unknown ";
    if (flags & ESM::Container::Organic)
        properties += "Organic ";
    if (flags & ESM::Container::Respawn)
        properties += "Respawn ";
    int unused = (0xFFFFFFFF ^ (ESM::Container::Unknown | ESM::Container::Organic | ESM::Container::Respawn));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string creatureFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Creature::Base)
        properties += "Base ";
    if (flags & ESM::Creature::Walks)
        properties += "Walks ";
    if (flags & ESM::Creature::Swims)
        properties += "Swims ";
    if (flags & ESM::Creature::Flies)
        properties += "Flies ";
    if (flags & ESM::Creature::Bipedal)
        properties += "Bipedal ";
    if (flags & ESM::Creature::Respawn)
        properties += "Respawn ";
    if (flags & ESM::Creature::Weapon)
        properties += "Weapon ";
    if (flags & ESM::Creature::Essential)
        properties += "Essential ";
    int unused = (0xFFFFFFFF
        ^ (ESM::Creature::Base | ESM::Creature::Walks | ESM::Creature::Swims | ESM::Creature::Flies
            | ESM::Creature::Bipedal | ESM::Creature::Respawn | ESM::Creature::Weapon | ESM::Creature::Essential));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:02X})", flags);
    return properties;
}

std::string enchantmentFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Enchantment::Autocalc)
        properties += "Autocalc ";
    if (flags & (0xFFFFFFFF ^ ESM::Enchantment::Autocalc))
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string landFlags(std::uint32_t flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Land::Flag_HeightsNormals)
        properties += "HeightsNormals ";
    if (flags & ESM::Land::Flag_Colors)
        properties += "Colors ";
    if (flags & ESM::Land::Flag_Textures)
        properties += "Textures ";
    int unused = 0xFFFFFFFF ^ (ESM::Land::Flag_HeightsNormals | ESM::Land::Flag_Colors | ESM::Land::Flag_Textures);
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string itemListFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::ItemLevList::AllLevels)
        properties += "AllLevels ";
    if (flags & ESM::ItemLevList::Each)
        properties += "Each ";
    int unused = (0xFFFFFFFF ^ (ESM::ItemLevList::AllLevels | ESM::ItemLevList::Each));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string creatureListFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::CreatureLevList::AllLevels)
        properties += "AllLevels ";
    int unused = (0xFFFFFFFF ^ ESM::CreatureLevList::AllLevels);
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string lightFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Light::Dynamic)
        properties += "Dynamic ";
    if (flags & ESM::Light::Fire)
        properties += "Fire ";
    if (flags & ESM::Light::Carry)
        properties += "Carry ";
    if (flags & ESM::Light::Flicker)
        properties += "Flicker ";
    if (flags & ESM::Light::FlickerSlow)
        properties += "FlickerSlow ";
    if (flags & ESM::Light::Pulse)
        properties += "Pulse ";
    if (flags & ESM::Light::PulseSlow)
        properties += "PulseSlow ";
    if (flags & ESM::Light::Negative)
        properties += "Negative ";
    if (flags & ESM::Light::OffDefault)
        properties += "OffDefault ";
    int unused = (0xFFFFFFFF
        ^ (ESM::Light::Dynamic | ESM::Light::Fire | ESM::Light::Carry | ESM::Light::Flicker | ESM::Light::FlickerSlow
            | ESM::Light::Pulse | ESM::Light::PulseSlow | ESM::Light::Negative | ESM::Light::OffDefault));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string magicEffectFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::MagicEffect::TargetAttribute)
        properties += "TargetAttribute ";
    if (flags & ESM::MagicEffect::TargetSkill)
        properties += "TargetSkill ";
    if (flags & ESM::MagicEffect::NoDuration)
        properties += "NoDuration ";
    if (flags & ESM::MagicEffect::NoMagnitude)
        properties += "NoMagnitude ";
    if (flags & ESM::MagicEffect::Harmful)
        properties += "Harmful ";
    if (flags & ESM::MagicEffect::ContinuousVfx)
        properties += "ContinuousVFX ";
    if (flags & ESM::MagicEffect::CastSelf)
        properties += "CastSelf ";
    if (flags & ESM::MagicEffect::CastTouch)
        properties += "CastTouch ";
    if (flags & ESM::MagicEffect::CastTarget)
        properties += "CastTarget ";
    if (flags & ESM::MagicEffect::AppliedOnce)
        properties += "AppliedOnce ";
    if (flags & ESM::MagicEffect::Stealth)
        properties += "Stealth ";
    if (flags & ESM::MagicEffect::NonRecastable)
        properties += "NonRecastable ";
    if (flags & ESM::MagicEffect::IllegalDaedra)
        properties += "IllegalDaedra ";
    if (flags & ESM::MagicEffect::Unreflectable)
        properties += "Unreflectable ";
    if (flags & ESM::MagicEffect::CasterLinked)
        properties += "CasterLinked ";
    if (flags & ESM::MagicEffect::AllowSpellmaking)
        properties += "AllowSpellmaking ";
    if (flags & ESM::MagicEffect::AllowEnchanting)
        properties += "AllowEnchanting ";
    if (flags & ESM::MagicEffect::NegativeLight)
        properties += "NegativeLight ";

    if (flags & 0xFFFC0000)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string npcFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::NPC::Base)
        properties += "Base ";
    if (flags & ESM::NPC::Autocalc)
        properties += "Autocalc ";
    if (flags & ESM::NPC::Female)
        properties += "Female ";
    if (flags & ESM::NPC::Respawn)
        properties += "Respawn ";
    if (flags & ESM::NPC::Essential)
        properties += "Essential ";
    // Whether corpses persist is a bit that is unaccounted for,
    // however relatively few NPCs have this bit set.
    int unused
        = (0xFF ^ (ESM::NPC::Base | ESM::NPC::Autocalc | ESM::NPC::Female | ESM::NPC::Respawn | ESM::NPC::Essential));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:02X})", flags);
    return properties;
}

std::string raceFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    // All races have the playable flag set in Bethesda files.
    if (flags & ESM::Race::Playable)
        properties += "Playable ";
    if (flags & ESM::Race::Beast)
        properties += "Beast ";
    int unused = (0xFFFFFFFF ^ (ESM::Race::Playable | ESM::Race::Beast));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string spellFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Spell::F_Autocalc)
        properties += "Autocalc ";
    if (flags & ESM::Spell::F_PCStart)
        properties += "PCStart ";
    if (flags & ESM::Spell::F_Always)
        properties += "Always ";
    int unused = (0xFFFFFFFF ^ (ESM::Spell::F_Autocalc | ESM::Spell::F_PCStart | ESM::Spell::F_Always));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string weaponFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    // The interpretation of the flags are still unclear to me.
    // Apparently you can't be Silver without being Magical?  Many of
    // the "Magical" weapons don't have enchantments of any sort.
    if (flags & ESM::Weapon::Magical)
        properties += "Magical ";
    if (flags & ESM::Weapon::Silver)
        properties += "Silver ";
    int unused = (0xFFFFFFFF ^ (ESM::Weapon::Magical | ESM::Weapon::Silver));
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string recordFlags(uint32_t flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::FLAG_Deleted)
        properties += "Deleted ";
    if (flags & ESM::FLAG_Persistent)
        properties += "Persistent ";
    if (flags & ESM::FLAG_Ignored)
        properties += "Ignored ";
    if (flags & ESM::FLAG_Blocked)
        properties += "Blocked ";
    int unused = ~(ESM::FLAG_Deleted | ESM::FLAG_Persistent | ESM::FLAG_Ignored | ESM::FLAG_Blocked);
    if (flags & unused)
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}

std::string potionFlags(int flags)
{
    std::string properties;
    if (flags == 0)
        properties += "[None] ";
    if (flags & ESM::Potion::Autocalc)
        properties += "Autocalc ";
    if (flags & (0xFFFFFFFF ^ ESM::Enchantment::Autocalc))
        properties += "Invalid ";
    properties += std::format("(0x{:08X})", flags);
    return properties;
}
