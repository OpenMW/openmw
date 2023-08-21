#include "attr.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <stdexcept>

namespace ESM
{
    const Attribute::AttributeID Attribute::Strength("Strength");
    const Attribute::AttributeID Attribute::Intelligence("Intelligence");
    const Attribute::AttributeID Attribute::Willpower("Willpower");
    const Attribute::AttributeID Attribute::Agility("Agility");
    const Attribute::AttributeID Attribute::Speed("Speed");
    const Attribute::AttributeID Attribute::Endurance("Endurance");
    const Attribute::AttributeID Attribute::Personality("Personality");
    const Attribute::AttributeID Attribute::Luck("Luck");

    static const RefId sAttributes[Attribute::Length] = {
        Attribute::Strength,
        Attribute::Intelligence,
        Attribute::Willpower,
        Attribute::Agility,
        Attribute::Speed,
        Attribute::Endurance,
        Attribute::Personality,
        Attribute::Luck,
    };

    RefId Attribute::indexToRefId(int index)
    {
        if (index < 0 || index >= Length)
            return RefId();
        return sAttributes[index];
    }

    int Attribute::refIdToIndex(RefId id)
    {
        for (int i = 0; i < Length; ++i)
        {
            if (sAttributes[i] == id)
                return i;
        }
        return -1;
    }

    void Attribute::load(ESMReader& esm, bool& isDeleted)
    {
        throw std::runtime_error("Attribute loading not yet implemented");
    }

    void Attribute::save(ESMWriter& esm, bool isDeleted) const
    {
        throw std::runtime_error("Attribute saving not yet implemented");
    }
}
