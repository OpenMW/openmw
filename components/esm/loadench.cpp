#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Enchantment::sRecordId = REC_ENCH;

    Enchantment::Enchantment()
        : mIsDeleted(false)
    {}

    void Enchantment::load(ESMReader &esm)
    {
        mEffects.mList.clear();
        mIsDeleted = false;

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t name = esm.retSubName().val;
            switch (name)
            {
                case ESM::FourCC<'N','A','M','E'>::value:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'D','E','L','E'>::value:
                    esm.skipHSub();
                    mIsDeleted = true;
                    break;
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

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !mIsDeleted)
            esm.fail("Missing ENDT subrecord");
    }

    void Enchantment::save(ESMWriter &esm) const
    {
        esm.writeHNCString("NAME", mId);

        if (mIsDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

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

        mIsDeleted = false;
    }
}
