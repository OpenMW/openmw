#include "loadspel.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Spell::sRecordId = REC_SPEL;

    void Spell::load(ESMReader &esm)
    {
        mEffects.mList.clear();
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t val = esm.retSubName().val;

            switch (val)
            {
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'S','P','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    ENAMstruct s;
                    esm.getHT(s, 24);
                    mEffects.mList.push_back(s);
                    break;
            }
        }
        if (!hasData)
            esm.fail("Missing SPDT subrecord");
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
