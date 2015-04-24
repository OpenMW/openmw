#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Enchantment::sRecordId = REC_ENCH;

void Enchantment::load(ESMReader &esm)
{
    mEffects.mList.clear();
    bool hasData = false;
    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'E','N','D','T'>::value:
                esm.getHT(mData, 16);
                hasData = true;
                break;
            case ESM::FourCC<'E','N','A','M'>::value:
                mEffects.add(esm);
                break;
            default:
                esm.fail("Unknown subrecord");
                break;
        }
    }
    if (!hasData)
        esm.fail("Missing ENDT subrecord");
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
