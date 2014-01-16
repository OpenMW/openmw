
#include "objectstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::ObjectState::load (ESMReader &esm)
{
    mRef.load (esm, true);

    mHasLocals = 0;
    esm.getHNOT (mHasLocals, "HLOC");

    if (mHasLocals)
        mLocals.load (esm);

    mEnabled = 1;
    esm.getHNOT (mEnabled, "ENAB");

    mCount = 1;
    esm.getHNOT (mCount, "COUN");

    esm.getHNT (mPosition, "POS_", 24);

    esm.getHNT (mLocalRotation, "LROT", 12);
}

void ESM::ObjectState::save (ESMWriter &esm) const
{
    mRef.save (esm, true);

    if (mHasLocals)
    {
        esm.writeHNT ("HLOC", mHasLocals);
        mLocals.save (esm);
    }

    if (!mEnabled)
        esm.writeHNT ("ENAB", mEnabled);

    if (mCount!=1)
        esm.writeHNT ("COUN", mCount);

    esm.writeHNT ("POS_", mPosition, 24);

    esm.writeHNT ("LROT", mLocalRotation, 12);
}