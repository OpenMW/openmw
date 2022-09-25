#ifndef OPENMW_ESM_GLOBALSCRIPT_H
#define OPENMW_ESM_GLOBALSCRIPT_H

#include "cellref.hpp"
#include "locals.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /// \brief Storage structure for global script state (only used in saved games)

    struct GlobalScript
    {
        RefId mId; /// \note must be lowercase
        Locals mLocals;
        int mRunning;
        RefId mTargetId; // for targeted scripts
        RefNum mTargetRef;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
