#include "attr.hpp"
#include <components/misc/strings/algorithm.hpp>
#include <stdexcept>

using namespace ESM;

const std::string Attribute::sAttributeNames[Attribute::Length] = {
    "Strength",
    "Intelligence",
    "Willpower",
    "Agility",
    "Speed",
    "Endurance",
    "Personality",
    "Luck",
};

Attribute::AttributeID Attribute::stringToAttributeId(std::string_view attribute)
{
    for (int id = 0; id < Attribute::Length; ++id)
        if (Misc::StringUtils::ciEqual(sAttributeNames[id], attribute))
            return Attribute::AttributeID(id);

    throw std::logic_error("No such attribute: " + std::string(attribute));
}
