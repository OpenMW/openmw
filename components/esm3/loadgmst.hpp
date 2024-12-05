#ifndef OPENMW_ESM_GMST_H
#define OPENMW_ESM_GMST_H

#include <string>

#include "variant.hpp"

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     *  Game setting
     *
     */

    struct GameSetting
    {
        constexpr static RecNameInts sRecordId = REC_GMST;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "GameSetting"; }

        uint32_t mRecordFlags;
        RefId mId;

        Variant mValue;

        void load(ESMReader& esm, bool& isDeleted);

        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };

    bool operator==(const GameSetting& left, const GameSetting& right);
}
#endif
