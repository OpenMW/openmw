#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Race::sRecordId = REC_RACE;

    int Race::MaleFemale::getValue (bool male) const
    {
        return male ? mMale : mFemale;
    }

    int Race::MaleFemaleF::getValue (bool male) const
    {
        return static_cast<int>(male ? mMale : mFemale);
    }

void Race::load(ESMReader &esm)
{
    mPowers.mList.clear();

    bool hasData = false;
    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'F','N','A','M'>::value:
                mName = esm.getHString();
                break;
            case ESM::FourCC<'R','A','D','T'>::value:
                esm.getHT(mData, 140);
                hasData = true;
                break;
            case ESM::FourCC<'D','E','S','C'>::value:
                mDescription = esm.getHString();
                break;
            case ESM::FourCC<'N','P','C','S'>::value:
                mPowers.add(esm);
                break;
            default:
                esm.fail("Unknown subrecord");
        }
    }
    if (!hasData)
        esm.fail("Missing RADT subrecord");
}
void Race::save(ESMWriter &esm) const
{
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("RADT", mData, 140);
    mPowers.save(esm);
    esm.writeHNOString("DESC", mDescription);
}

    void Race::blank()
    {
        mName.clear();
        mDescription.clear();

        mPowers.mList.clear();

        for (int i=0; i<7; ++i)
        {
            mData.mBonus[i].mSkill = -1;
            mData.mBonus[i].mBonus = 0;
        }

        for (int i=0; i<8; ++i)
            mData.mAttributeValues[i].mMale = mData.mAttributeValues[i].mFemale = 1;

        mData.mHeight.mMale = mData.mHeight.mFemale = 1;
        mData.mWeight.mMale = mData.mWeight.mFemale = 1;

        mData.mFlags = 0;
    }
}
