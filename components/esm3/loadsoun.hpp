#ifndef OPENMW_ESM_SOUN_H
#define OPENMW_ESM_SOUN_H

#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    struct SOUNstruct
    {
        unsigned char mVolume, mMinRange, mMaxRange;
    };

    struct Sound
    {
        constexpr static RecNameInts sRecordId = REC_SOUN;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Sound"; }

        SOUNstruct mData;
        uint32_t mRecordFlags;
        std::string mSound;
        RefId mId;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).
    };
}
#endif
