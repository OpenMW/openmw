#ifndef OPENMW_ESM_MISC_H
#define OPENMW_ESM_MISC_H

#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Misc inventory items, basically things that have no use but can be
     * carried, bought and sold. It also includes keys.
     */

    struct Miscellaneous
    {
        constexpr static RecNameInts sRecordId = REC_MISC;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Miscellaneous"; }

        struct MCDTstruct
        {
            float mWeight;
            int32_t mValue;
            int32_t mFlags;
        };

        enum Flags
        {
            Key = 0x1 // Assigned as a key in the content file that defined the record
        };

        MCDTstruct mData;

        uint32_t mRecordFlags;
        RefId mId, mScript;
        std::string mName, mModel, mIcon;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}
#endif
