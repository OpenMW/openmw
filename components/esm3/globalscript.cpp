#include "globalscript.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void GlobalScript::load(ESMReader& esm)
    {
        mId = esm.getHNRefId("NAME");

        mLocals.load(esm);

        mRunning = 0;
        esm.getHNOT(mRunning, "RUN_");

        mTargetRef = RefNum{};
        mTargetId = esm.getHNORefId("TARG");
        if (esm.peekNextSub("FRMR"))
            mTargetRef = esm.getFormId(true, "FRMR");
    }

    void GlobalScript::save(ESMWriter& esm) const
    {
        esm.writeHNRefId("NAME", mId);

        mLocals.save(esm);

        if (mRunning)
            esm.writeHNT("RUN_", mRunning);

        if (!mTargetId.empty())
        {
            esm.writeHNORefId("TARG", mTargetId);
            if (mTargetRef.isSet())
                esm.writeFormId(mTargetRef, true, "FRMR");
        }
    }

}
