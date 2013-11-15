#include "loadweap.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Weapon::sRecordId = REC_WEAP;

void Weapon::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "WPDT", 32);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
    mEnchant = esm.getHNOString("ENAM");
}
void Weapon::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("WPDT", mData, 32);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
    esm.writeHNOCString("ENAM", mEnchant);
}

    void Weapon::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mType = 0;
        mData.mHealth = 0;
        mData.mSpeed = 0;
        mData.mReach = 0;
        mData.mEnchant = 0;
        mData.mChop[0] = mData.mChop[1] = 0;
        mData.mSlash[0] = mData.mSlash[1] = 0;
        mData.mThrust[0] = mData.mThrust[1] = 0;
        mData.mFlags = 0;

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant.clear();
        mScript.clear();
    }
}
