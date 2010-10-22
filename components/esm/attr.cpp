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
