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
        return male ? mMale : mFemale;
    }

void Race::load(ESMReader &esm)
{
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "RADT", 140);
    mPowers.load(esm);
    mDescription = esm.getHNOString("DESC");
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
