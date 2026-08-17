#include "attr.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <stdexcept>

namespace ESM
{
    const RefId Attribute::Strength(ESM::StringRefId("Strength"));
    const RefId Attribute::Intelligence(ESM::StringRefId("Intelligence"));
    const RefId Attribute::Willpower(ESM::StringRefId("Willpower"));
    const RefId Attribute::Agility(ESM::StringRefId("Agility"));
    const RefId Attribute::Speed(ESM::StringRefId("Speed"));
    const RefId Attribute::Endurance(ESM::StringRefId("Endurance"));
    const RefId Attribute::Personality(ESM::StringRefId("Personality"));
    const RefId Attribute::Luck(ESM::StringRefId("Luck"));

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

    void Attribute::blank()
    {
        mId = {};
        mName.clear();
        mDescription.clear();
        mIcon.clear();
        mWerewolfValue = {};
    }
}
