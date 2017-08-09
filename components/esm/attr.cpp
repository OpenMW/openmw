#include "attr.hpp"

using namespace ESM;

const Attribute::AttributeID Attribute::sAttributeIds[Attribute::Length] = {
    Attribute::Strength,
    Attribute::Intelligence,
    Attribute::Willpower,
    Attribute::Agility,
    Attribute::Speed,
    Attribute::Endurance,
    Attribute::Personality,
    Attribute::Luck
};

const std::string Attribute::sAttributeNames[Attribute::Length] = {
    "Strength",
    "Intelligence",
    "Willpower",
    "Agility",
    "Speed",
    "Endurance",
    "Personality",
    "Luck"
};

const std::string Attribute::sGmstAttributeIds[Attribute::Length] = {
    "sAttributeStrength",
    "sAttributeIntelligence",
    "sAttributeWillpower",
    "sAttributeAgility",
    "sAttributeSpeed",
    "sAttributeEndurance",
    "sAttributePersonality",
    "sAttributeLuck"
};

const std::string Attribute::sGmstAttributeDescIds[Attribute::Length] = {
    "sStrDesc",
    "sIntDesc",
    "sWilDesc",
    "sAgiDesc",
    "sSpdDesc",
    "sEndDesc",
    "sPerDesc",
    "sLucDesc"
};

const std::string Attribute::sAttributeIcons[Attribute::Length] = {
    "icons\\k\\attribute_strength.dds",
    "icons\\k\\attribute_int.dds",
    "icons\\k\\attribute_wilpower.dds",
    "icons\\k\\attribute_agility.dds",
    "icons\\k\\attribute_speed.dds",
    "icons\\k\\attribute_endurance.dds",
    "icons\\k\\attribute_personality.dds",
    "icons\\k\\attribute_luck.dds"
};
