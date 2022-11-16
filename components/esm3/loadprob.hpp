#ifndef OPENMW_ESM_PROBE_H
#define OPENMW_ESM_PROBE_H

#include <string>
#include <string_view>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    struct Probe
    {
        constexpr static RecNameInts sRecordId = REC_PROB;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Probe"; }

        struct Data
        {
            float mWeight;
            int mValue;

            float mQuality;
            int mUses;
        }; // Size = 16

        Data mData;
        unsigned int mRecordFlags;
        RefId mId, mScript;
        std::string mName, mModel, mIcon;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };

}
#endif
