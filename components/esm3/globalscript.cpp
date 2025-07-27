#include "globalscript.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <cstdint>

namespace ESM
{

    void GlobalScript::load(ESMReader& esm)
    {
        mId = esm.getHNRefId("NAME");

        mLocals.load(esm);

        int32_t running = 0;
        esm.getHNOT(running, "RUN_");
        mRunning = running != 0;

        mTargetRef = RefNum{};
        mTargetId = esm.getHNORefId("TARG");
        if (esm.peekNextSub("FRMR"))
            mTargetRef = esm.getFormId(true, "FRMR");
        if (!esm.applyContentFileMapping(mTargetRef))
        {
            mTargetId = ESM::RefId();
            mTargetRef = ESM::FormId();
        }
    }

    void GlobalScript::save(ESMWriter& esm) const
    {
        esm.writeHNRefId("NAME", mId);

        mLocals.save(esm);

        if (mRunning)
            esm.writeHNT("RUN_", int32_t{ 1 });

        esm.writeHNORefId("TARG", mTargetId);
        if (mTargetRef.isSet())
            esm.writeFormId(mTargetRef, true, "FRMR");
    }

}
