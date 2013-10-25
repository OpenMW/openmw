#include "loadspel.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Spell::sRecordId = REC_SPEL;

void Spell::load(ESMReader &esm)
{
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "SPDT", 12);
    mEffects.load(esm);
}

void Spell::save(ESMWriter &esm) const
{
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("SPDT", mData, 12);
    mEffects.save(esm);
}

    void Spell::blank()
    {
        mData.mType = 0;
        mData.mCost = 0;
        mData.mFlags = 0;

        mName.clear();

        mEffects.mList.clear();
    }
}
