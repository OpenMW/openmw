#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void Enchantment::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();
        mEffects.mList.clear();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case fourCC("ENDT"):
                    esm.getHTSized<16>(mData);
                    hasData = true;
                    break;
                case fourCC("ENAM"):
                    mEffects.add(esm);
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing ENDT subrecord");
    }

    void Enchantment::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("ENDT", mData, 16);
        mEffects.save(esm);
    }

    void Enchantment::blank()
    {
        mRecordFlags = 0;
        mData.mType = 0;
        mData.mCost = 0;
        mData.mCharge = 0;
        mData.mFlags = 0;

        mEffects.mList.clear();
    }
}
