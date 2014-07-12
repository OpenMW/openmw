#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Enchantment::sRecordId = REC_ENCH;

void Enchantment::load(ESMReader &esm)
{
    esm.getHNT(mData, "ENDT", 16);
    mEffects.load(esm);
}

void Enchantment::save(ESMWriter &esm) const
{
    esm.writeHNT("ENDT", mData, 16);
    mEffects.save(esm);
}

    void Enchantment::blank()
    {
        mData.mType = 0;
        mData.mCost = 0;
        mData.mCharge = 0;
        mData.mAutocalc = 0;

        mEffects.mList.clear();
    }
}
