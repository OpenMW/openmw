#include "globalscript.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::GlobalScript::load (ESMReader &esm)
{
    mId = esm.getHNString ("NAME");

    mLocals.load (esm);

    mRunning = 0;
    esm.getHNOT (mRunning, "RUN_");

    mTargetId = esm.getHNOString ("TARG");
}

void ESM::GlobalScript::save (ESMWriter &esm) const
{
    esm.writeHNString ("NAME", mId);

    mLocals.save (esm);

    if (mRunning)
        esm.writeHNT ("RUN_", mRunning);

    esm.writeHNOString ("TARG", mTargetId);
}
