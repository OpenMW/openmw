#ifndef OPENMW_ESM_CLOT_H
#define OPENMW_ESM_CLOT_H

#include <cstdint>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "loadarmo.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Clothing
     */

    struct Clothing
    {
        constexpr static RecNameInts sRecordId = REC_CLOT;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Clothing"; }

        enum Type
        {
            Pants = 0,
            Shoes = 1,
            Shirt = 2,
            Belt = 3,
            Robe = 4,
            RGlove = 5,
            LGlove = 6,
            Skirt = 7,
            Ring = 8,
            Amulet = 9
        };

        struct CTDTstruct
        {
            int32_t mType;
            float mWeight;
            uint16_t mValue;
            uint16_t mEnchant;
        };
        CTDTstruct mData;

        PartReferenceList mParts;

        uint32_t mRecordFlags;
        RefId mId, mEnchant, mScript;
        std::string mModel, mIcon, mName;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}
#endif
