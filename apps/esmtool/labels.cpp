#include "labels.hpp"

#include <components/esm/loadbody.hpp>
#include <components/esm/loadcell.hpp>
#include <components/esm/loadcont.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/loadlevlist.hpp>
#include <components/esm/loadligh.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/aipackage.hpp>

#include <iostream>
#include <boost/format.hpp>

std::string bodyPartLabel(int idx)
{
    const char *bodyPartLabels[] =  {
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
        "Tail"
    };

    if (idx >= 0 && idx <= 26)
        return bodyPartLabels[idx];
    else
        return "Invalid";
}

std::string meshPartLabel(int idx)
{
    const char *meshPartLabels[] =  {
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
        "Tail"
    };

    if (idx >= 0 && idx <= ESM::BodyPart::MP_Tail)
        return meshPartLabels[idx];
    else
        return "Invalid";
}

std::string meshTypeLabel(int idx)
{
    const char *meshTypeLabels[] =  {
        "Skin",
        "Clothing",
        "Armor"
    };

    if (idx >= 0 && idx <= ESM::BodyPart::MT_Armor)
        return meshTypeLabels[idx];
    else
        return "Invalid";
}

std::string clothingTypeLabel(int idx)
{
    const char *clothingTypeLabels[] = {
        "Pants",
        "Shoes",
        "Shirt",
        "Belt",
        "Robe",
        "Right Glove",
        "Left Glove",
        "Skirt",
        "Ring",
        "Amulet"
    };

    if (idx >= 0 && idx <= 9)
        return clothingTypeLabels[idx];
    else
        return "Invalid";
}

std::string armorTypeLabel(int idx)
{
    const char *armorTypeLabels[] = {
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
        "Right Bracer"
    };
    
    if (idx >= 0 && idx <= 10)
        return armorTypeLabels[idx];
    else
        return "Invalid";
}

std::string dialogTypeLabel(int idx)
{
    const char *dialogTypeLabels[] = {
        "Topic",
        "Voice",
        "Greeting",
        "Persuasion",
        "Journal"
    };
    
    if (idx >= 0 && idx <= 4)
        return dialogTypeLabels[idx];
    else if (idx == -1)
        return "Deleted";
    else
        return "Invalid";
}

std::string questStatusLabel(int idx)
{
    const char *questStatusLabels[] = {
        "None",
        "Name",
        "Finished",
        "Restart",
        "Deleted"
    };
    
    if (idx >= 0 && idx <= 4)
        return questStatusLabels[idx];
    else
        return "Invalid";
}

std::string creatureTypeLabel(int idx)
{
    const char *creatureTypeLabels[] = {
        "Creature",
        "Daedra",
        "Undead",
        "Humanoid",
    };
    
    if (idx >= 0 && idx <= 3)
        return creatureTypeLabels[idx];
    else
        return "Invalid";
}

std::string soundTypeLabel(int idx)
{
    const char *soundTypeLabels[] = {
        "Left Foot",
        "Right Foot",
        "Swim Left",
        "Swim Right",
        "Moan",
        "Roar",
        "Scream",
        "Land"
    };
    
    if (idx >= 0 && idx <= 7)
        return soundTypeLabels[idx];
    else
        return "Invalid";
}

std::string weaponTypeLabel(int idx)
{
    const char *weaponTypeLabels[] = {
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
        "Bolt"
    };
    
    if (idx >= 0 && idx <= 13)
        return weaponTypeLabels[idx];
    else
        return "Invalid";
}

std::string aiTypeLabel(int type)
{
    if (type == ESM::AI_Wander) return "Wander";
    else if (type == ESM::AI_Travel) return "Travel";
    else if (type == ESM::AI_Follow) return "Follow";
    else if (type == ESM::AI_Escort) return "Escort";
    else if (type == ESM::AI_Activate) return "Activate";
    else return "Invalid";
}

std::string magicEffectLabel(int idx)
{
    const char* magicEffectLabels [] = {
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
        "sEffectSummonCreature05"
    };
    if (idx >= 0 && idx <= 143)
        return magicEffectLabels[idx];
    else
        return "Invalid";
}

std::string attributeLabel(int idx)
{
    const char* attributeLabels [] = {
        "Strength",
        "Intelligence",
        "Willpower",
        "Agility",
        "Speed",
        "Endurance",
        "Personality",
        "Luck"
    };
    if (idx >= 0 && idx <= 7)
        return attributeLabels[idx];
    else
        return "Invalid";
}

std::string spellTypeLabel(int idx)
{
    const char* spellTypeLabels [] = {
        "Spells",
        "Abilities",
        "Blight Disease",
        "Disease",
        "Curse",
        "Powers"
    };
    if (idx >= 0 && idx <= 5)
        return spellTypeLabels[idx];
    else
        return "Invalid";
}

std::string specializationLabel(int idx)
{
    const char* specializationLabels [] = {
        "Combat",
        "Magic",
        "Stealth"
    };
    if (idx >= 0 && idx <= 2)
        return specializationLabels[idx];
    else
        return "Invalid";
}

std::string skillLabel(int idx)
{
    const char* skillLabels [] = {
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
        "Hand-to-hand"
    };
    if (idx >= 0 && idx <= 27)
        return skillLabels[idx];
    else
        return "Invalid";
}

std::string apparatusTypeLabel(int idx)
{
    const char* apparatusTypeLabels [] = {
        "Mortar",
        "Alembic",
        "Calcinator",
        "Retort",
    };
    if (idx >= 0 && idx <= 3)
        return apparatusTypeLabels[idx];
    else
        return "Invalid";
}

std::string rangeTypeLabel(int idx)
{
    const char* rangeTypeLabels [] = {
        "Self",
        "Touch",
        "Target"
    };
    if (idx >= 0 && idx <= 3)
        return rangeTypeLabels[idx];
    else
        return "Invalid";
}

std::string schoolLabel(int idx)
{
    const char* schoolLabels [] = {
        "Alteration",
        "Conjuration",
        "Destruction",
        "Illusion",
        "Mysticism",
        "Restoration"
    };
    if (idx >= 0 && idx <= 5)
        return schoolLabels[idx];
    else
        return "Invalid";
}

std::string enchantTypeLabel(int idx)
{
    const char* enchantTypeLabels [] = {
        "Cast Once",
        "Cast When Strikes",
        "Cast When Used",
        "Constant Effect"
    };
    if (idx >= 0 && idx <= 3)
        return enchantTypeLabels[idx];
    else
        return "Invalid";
}

std::string ruleFunction(int idx)
{
    std::string ruleFunctions[] = {
        "Reaction Low",
        "Reaction High",
        "Rank Requirement",
        "NPC? Reputation",
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
        //Unkown but causes NPCs to growl and roar.
        "UNKNOWN 72"
    };
    if (idx >= 0 && idx <= 72)
        return ruleFunctions[idx];
    else
        return "Invalid";
}
   
// The "unused flag bits" should probably be defined alongside the
// defined bits in the ESM component.  The names of the flag bits are
// very inconsistent.

std::string bodyPartFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";    
    if (flags & ESM::BodyPart::BPF_Female) properties += "Female ";
    if (flags & ESM::BodyPart::BPF_NotPlayable) properties += "NotPlayable ";
    int unused = (0xFFFFFFFF ^
                  (ESM::BodyPart::BPF_Female|
                   ESM::BodyPart::BPF_NotPlayable));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string cellFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";    
    if (flags & ESM::Cell::HasWater) properties += "HasWater ";
    if (flags & ESM::Cell::Interior) properties += "Interior ";
    if (flags & ESM::Cell::NoSleep) properties += "NoSleep ";
    if (flags & ESM::Cell::QuasiEx) properties += "QuasiEx ";
    // This used value is not in the ESM component.
    if (flags & 0x00000040) properties += "Unknown ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Cell::HasWater|
                   ESM::Cell::Interior|
                   ESM::Cell::NoSleep|
                   ESM::Cell::QuasiEx|
                   0x00000040));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string containerFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    if (flags & ESM::Container::Unknown) properties += "Unknown ";
    if (flags & ESM::Container::Organic) properties += "Organic ";
    if (flags & ESM::Container::Respawn) properties += "Respawn ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Container::Unknown|
                   ESM::Container::Organic|
                   ESM::Container::Respawn));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string creatureFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    if (flags & ESM::Creature::None) properties += "All ";
    if (flags & ESM::Creature::Walks) properties += "Walks ";
    if (flags & ESM::Creature::Swims) properties += "Swims ";
    if (flags & ESM::Creature::Flies) properties += "Flies ";
    if (flags & ESM::Creature::Biped) properties += "Biped ";   
    if (flags & ESM::Creature::Respawn) properties += "Respawn ";
    if (flags & ESM::Creature::Weapon) properties += "Weapon ";
    if (flags & ESM::Creature::Skeleton) properties += "Skeleton ";
    if (flags & ESM::Creature::Metal) properties += "Metal ";
    if (flags & ESM::Creature::Essential) properties += "Essential ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Creature::None|
                   ESM::Creature::Walks|
                   ESM::Creature::Swims|
                   ESM::Creature::Flies|
                   ESM::Creature::Biped|
                   ESM::Creature::Respawn|
                   ESM::Creature::Weapon|
                   ESM::Creature::Skeleton|
                   ESM::Creature::Metal|
                   ESM::Creature::Essential));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string landFlags(int flags)
{
    std::string properties = "";
    // The ESM component says that this first four bits are used, but
    // only the first three bits are used as far as I can tell.
    // There's also no enumeration of the bit in the ESM component.
    if (flags == 0) properties += "[None] ";
    if (flags & 0x00000001) properties += "Unknown1 ";
    if (flags & 0x00000004) properties += "Unknown3 ";
    if (flags & 0x00000002) properties += "Unknown2 ";
    if (flags & 0xFFFFFFF8) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string leveledListFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    if (flags & ESM::LeveledListBase::AllLevels) properties += "AllLevels ";
    // This flag apparently not present on creature lists...
    if (flags & ESM::LeveledListBase::Each) properties += "Each ";
    int unused = (0xFFFFFFFF ^
                  (ESM::LeveledListBase::AllLevels|
                   ESM::LeveledListBase::Each));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string lightFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    if (flags & ESM::Light::Dynamic) properties += "Dynamic ";
    if (flags & ESM::Light::Fire) properties += "Fire ";
    if (flags & ESM::Light::Carry) properties += "Carry ";
    if (flags & ESM::Light::Flicker) properties += "Flicker ";
    if (flags & ESM::Light::FlickerSlow) properties += "FlickerSlow ";
    if (flags & ESM::Light::Pulse) properties += "Pulse ";
    if (flags & ESM::Light::PulseSlow) properties += "PulseSlow ";
    if (flags & ESM::Light::Negative) properties += "Negative ";
    if (flags & ESM::Light::OffDefault) properties += "OffDefault ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Light::Dynamic|
                   ESM::Light::Fire|
                   ESM::Light::Carry|
                   ESM::Light::Flicker|
                   ESM::Light::FlickerSlow|
                   ESM::Light::Pulse|
                   ESM::Light::PulseSlow|
                   ESM::Light::Negative|
                   ESM::Light::OffDefault));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string magicEffectFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    // Enchanting & SpellMaking occur on the same list of effects.
    // "EXTRA SPELL" appears in the construction set under both the
    // spell making and enchanting tabs as an allowed effect.  Since
    // most of the effects without this flags are defective in various
    // ways, it's still very unclear what these flag bits are.
    if (flags & ESM::MagicEffect::SpellMaking) properties += "SpellMaking ";
    if (flags & ESM::MagicEffect::Enchanting) properties += "Enchanting ";
    if (flags & 0x00000040) properties += "RangeNoSelf ";
    if (flags & 0x00000080) properties += "RangeTouch ";
    if (flags & 0x00000100) properties += "RangeTarget ";
    if (flags & 0x00001000) properties += "Unknown2 ";
    if (flags & 0x00000001) properties += "AffectSkill ";
    if (flags & 0x00000002) properties += "AffectAttribute ";
    if (flags & ESM::MagicEffect::NoDuration) properties += "NoDuration ";
    if (flags & 0x00000008) properties += "NoMagnitude ";
    if (flags & 0x00000010) properties += "Negative ";
    if (flags & 0x00000020) properties += "Unknown1 ";
    // ESM componet says 0x800 is negative, but none of the magic
    // effects have this flags set.
    if (flags & ESM::MagicEffect::Negative) properties += "Unused ";
    // Since only Chameleon has this flag it could be anything
    // that uniquely distinguishes Chameleon.
    if (flags & 0x00002000) properties += "Chameleon ";
    if (flags & 0x00004000) properties += "Bound ";
    if (flags & 0x00008000) properties += "Summon ";
    // Calm, Demoralize, Frenzy, Lock, Open, Rally, Soultrap, Turn Unded
    if (flags & 0x00010000) properties += "Unknown3 ";
    if (flags & 0x00020000) properties += "Absorb ";
    if (flags & 0xFFFC0000) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string npcFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    // Mythicmods and the ESM component differ.  Mythicmods says
    // 0x8=None and 0x10=AutoCalc, while our code defines 0x8 as
    // AutoCalc.  The former seems to be correct.  All Bethesda
    // records have bit 0x8 set.  A suspiciously large portion of
    // females have autocalc turned off.
    if (flags & ESM::NPC::Autocalc) properties += "Unknown ";
    if (flags & 0x00000010) properties += "Autocalc ";
    if (flags & ESM::NPC::Female) properties += "Female ";
    if (flags & ESM::NPC::Respawn) properties += "Respawn ";
    if (flags & ESM::NPC::Essential) properties += "Essential ";
    // These two flags do not appear on any NPCs and may have been
    // confused with the flags for creatures.
    if (flags & ESM::NPC::Skeleton) properties += "Skeleton ";
    if (flags & ESM::NPC::Metal) properties += "Metal ";
    // Whether corpses persist is a bit that is unaccounted for,
    // however the only unknown bit occurs on ALL records, and
    // relatively few NPCs have this bit set.
    int unused = (0xFFFFFFFF ^
                  (ESM::NPC::Autocalc|
                   0x00000010|
                   ESM::NPC::Female|
                   ESM::NPC::Respawn|
                   ESM::NPC::Essential|
                   ESM::NPC::Skeleton|
                   ESM::NPC::Metal));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string raceFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    // All races have the playable flag set in Bethesda files.
    if (flags & ESM::Race::Playable) properties += "Playable ";
    if (flags & ESM::Race::Beast) properties += "Beast ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Race::Playable|
                   ESM::Race::Beast));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string spellFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    if (flags & ESM::Spell::F_Autocalc) properties += "Autocalc ";
    if (flags & ESM::Spell::F_PCStart) properties += "PCStart ";
    if (flags & ESM::Spell::F_Always) properties += "Always ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Spell::F_Autocalc|
                   ESM::Spell::F_PCStart|
                   ESM::Spell::F_Always));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}

std::string weaponFlags(int flags)
{
    std::string properties = "";
    if (flags == 0) properties += "[None] ";
    // The interpretation of the flags are still unclear to me.
    // Apparently you can't be Silver without being Magical?  Many of
    // the "Magical" weapons don't have enchantments of any sort.
    if (flags & ESM::Weapon::Magical) properties += "Magical ";
    if (flags & ESM::Weapon::Silver) properties += "Silver ";
    int unused = (0xFFFFFFFF ^
                  (ESM::Weapon::Magical|
                   ESM::Weapon::Silver));
    if (flags & unused) properties += "Invalid ";
    properties += str(boost::format("(0x%08X)") % flags);
    return properties;
}
