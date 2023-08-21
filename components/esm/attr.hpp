#ifndef OPENMW_ESM_ATTR_H
#define OPENMW_ESM_ATTR_H

#include <string>
#include <string_view>

#include "defs.hpp"
#include "refid.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /*
     * Attribute definitions
     */

    struct Attribute
    {
        constexpr static RecNameInts sRecordId = REC_ATTR;
        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Attribute"; }

        using AttributeID = StringRefId;
        static const AttributeID Strength;
        static const AttributeID Intelligence;
        static const AttributeID Willpower;
        static const AttributeID Agility;
        static const AttributeID Speed;
        static const AttributeID Endurance;
        static const AttributeID Personality;
        static const AttributeID Luck;
        static constexpr int Length = 8;

        AttributeID mId;
        std::string mName, mDescription, mIcon;
        float mWerewolfValue{};

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        static RefId indexToRefId(int index);
        static int refIdToIndex(RefId id);
    };
}
#endif
