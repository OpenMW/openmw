#ifndef OPENMW_ESM_GLOBALSCRIPT_H
#define OPENMW_ESM_GLOBALSCRIPT_H

#include "locals.hpp"
#include "cellref.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /// \brief Storage structure for global script state (only used in saved games)

    struct GlobalScript
    {
        std::string mId; /// \note must be lowercase
        Locals mLocals;
        int mRunning;
        std::string mTargetId; // for targeted scripts
        RefNum mTargetRef;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
