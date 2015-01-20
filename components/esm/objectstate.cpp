
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

    esm.getHNOT (mPosition, "POS_", 24);

    esm.getHNOT (mLocalRotation, "LROT", 12);
}

void ESM::ObjectState::save (ESMWriter &esm, bool inInventory) const
{
    mRef.save (esm, true, inInventory);

    if (mHasLocals)
    {
        esm.writeHNT ("HLOC", mHasLocals);
        mLocals.save (esm);
    }

    if (!mEnabled && !inInventory)
        esm.writeHNT ("ENAB", mEnabled);

    if (mCount!=1)
        esm.writeHNT ("COUN", mCount);

    if (!inInventory)
    {
        esm.writeHNT ("POS_", mPosition, 24);
        esm.writeHNT ("LROT", mLocalRotation, 12);
    }
}

void ESM::ObjectState::blank()
{
    mRef.blank();
    mHasLocals = 0;
    mEnabled = false;
    mCount = 1;
    for (int i=0;i<3;++i)
    {
        mPosition.pos[i] = 0;
        mPosition.rot[i] = 0;
        mLocalRotation[i] = 0;
    }
}

ESM::ObjectState::~ObjectState() {}
