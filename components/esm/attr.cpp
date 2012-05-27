#include "attr.hpp"

using namespace ESM;

const Attribute::AttributeID Attribute::attributeIds[Attribute::Length] = {
    Attribute::Strength,
    Attribute::Intelligence,
    Attribute::Willpower,
    Attribute::Agility,
    Attribute::Speed,
    Attribute::Endurance,
    Attribute::Personality,
    Attribute::Luck
};

const std::string Attribute::gmstAttributeIds[Attribute::Length] = {
    "sAttributeStrength",
    "sAttributeIntelligence",
    "sAttributeWillpower",
    "sAttributeAgility",
    "sAttributeSpeed",
    "sAttributeEndurance",
    "sAttributePersonality",
    "sAttributeLuck"
};

const std::string Attribute::gmstAttributeDescIds[Attribute::Length] = {
    "sStrDesc",
    "sIntDesc",
    "sWilDesc",
    "sAgiDesc",
    "sSpdDesc",
    "sEndDesc",
    "sPerDesc",
    "sLucDesc"
};

const std::string Attribute::attributeIcons[Attribute::Length] = {
    "icons\\k\\attribute_strength.dds",
    "icons\\k\\attribute_int.dds",
    "icons\\k\\attribute_wilpower.dds",
    "icons\\k\\attribute_agility.dds",
    "icons\\k\\attribute_speed.dds",
    "icons\\k\\attribute_endurance.dds",
    "icons\\k\\attribute_personality.dds",
    "icons\\k\\attribute_luck.dds"
};
