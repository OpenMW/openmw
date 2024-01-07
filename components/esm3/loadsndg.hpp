#ifndef OPENMW_ESM_SNDG_H
#define OPENMW_ESM_SNDG_H

#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Sound generator. This describes the sounds a creature make.
     */

    struct SoundGenerator
    {
        constexpr static RecNameInts sRecordId = REC_SNDG;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "SoundGenerator"; }

        enum Type
        {
            LeftFoot = 0,
            RightFoot = 1,
            SwimLeft = 2,
            SwimRight = 3,
            Moan = 4,
            Roar = 5,
            Scream = 6,
            Land = 7
        };

        // Type
        int32_t mType;

        uint32_t mRecordFlags;
        RefId mId, mCreature, mSound;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
    };
}
#endif
