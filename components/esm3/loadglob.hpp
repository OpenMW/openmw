#ifndef OPENMW_ESM_GLOB_H
#define OPENMW_ESM_GLOB_H

#include <string>

#include "variant.hpp"

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Global script variables
     */

    struct Global
    {
        constexpr static RecNameInts sRecordId = REC_GLOB;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Global"; }

        uint32_t mRecordFlags;
        ESM::RefId mId;
        Variant mValue;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };

    bool operator==(const Global& left, const Global& right);

}
#endif
