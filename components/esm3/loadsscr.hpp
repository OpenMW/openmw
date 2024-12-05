#ifndef OPENMW_ESM_SSCR_H
#define OPENMW_ESM_SSCR_H

#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     Startup script. I think this is simply a 'main' script that is run
     from the begining. The SSCR records contain a DATA identifier which
     is totally useless (TODO: don't remember what it contains exactly,
     document it below later.), and a NAME which is simply a script
     reference.
     */

    struct StartScript
    {
        constexpr static RecNameInts sRecordId = REC_SSCR;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "StartScript"; }

        std::string mData;
        uint32_t mRecordFlags;
        RefId mId;

        // Load a record and add it to the list
        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
    };

}
#endif
