
#include "objectstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::ObjectState::load (ESMReader &esm)
{
    mRef.loadData(esm);

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

    // obsolete
    int unused;
    esm.getHNOT(unused, "LTIM");

    // FIXME: assuming "false" as default would make more sense, but also break compatibility with older save files
    mHasCustomState = true;
    esm.getHNOT (mHasCustomState, "HCUS");
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

    if (!mHasCustomState)
        esm.writeHNT ("HCUS", false);
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
    mHasCustomState = true;
}

ESM::ObjectState::~ObjectState() {}
