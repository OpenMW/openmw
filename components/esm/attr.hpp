#ifndef OPENMW_ESM_ATTR_H
#define OPENMW_ESM_ATTR_H

#include <string>

namespace ESM {

/*
 * Attribute definitions
 */

struct Attribute
{
    enum AttributeID
    {
        Strength = 0,
        Intelligence = 1,
        Willpower = 2,
        Agility = 3,
        Speed = 4,
        Endurance = 5,
        Personality = 6,
        Luck = 7,
        Length
    };

    AttributeID mId;
    std::string mName, mDescription;

    static const AttributeID sAttributeIds[Length];
    static const std::string sAttributeNames[Length];
    static const std::string sGmstAttributeIds[Length];
    static const std::string sGmstAttributeDescIds[Length];
    static const std::string sAttributeIcons[Length];
};
}
#endif
