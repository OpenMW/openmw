#include "globalscript.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::GlobalScript::load (ESMReader &esm)
{
    mId = esm.getHNString ("NAME");

    mLocals.load (esm);

    mRunning = 0;
    esm.getHNOT (mRunning, "RUN_");

    mTargetRef.unset();
    mTargetId = esm.getHNOString ("TARG");
    if (esm.peekNextSub("FRMR"))
        mTargetRef.load(esm, true, "FRMR");
}

void ESM::GlobalScript::save (ESMWriter &esm) const
{
    esm.writeHNString ("NAME", mId);

    mLocals.save (esm);

    if (mRunning)
        esm.writeHNT ("RUN_", mRunning);

    if (!mTargetId.empty())
    {
        esm.writeHNOString ("TARG", mTargetId);
        if (mTargetRef.isSet())
            mTargetRef.save (esm, true, "FRMR");
    }
}
