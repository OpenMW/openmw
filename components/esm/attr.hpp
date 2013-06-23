#ifndef _ESM_ATTR_H
#define _ESM_ATTR_H

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

    AttributeID id;
    std::string name, description;

    static const AttributeID attributeIds[Length];
    static const std::string gmstAttributeIds[Length];
    static const std::string gmstAttributeDescIds[Length];

    Attribute(AttributeID id, const std::string &name, const std::string &description)
        : id(id)
        , name(name)
        , description(description)
    {
    }
};
}
#endif
