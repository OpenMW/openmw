#include "loadarmo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{

void PartReferenceList::load(ESMReader &esm)
{
    while (esm.isNextSub("INDX"))
    {
        PartReference pr;
        esm.getHT(pr.mPart); // The INDX byte
        pr.mMale = esm.getHNOString("BNAM");
        pr.mFemale = esm.getHNOString("CNAM");
        mParts.push_back(pr);
    }
}

void PartReferenceList::save(ESMWriter &esm) const
{
    for (std::vector<PartReference>::const_iterator it = mParts.begin(); it != mParts.end(); ++it)
    {
        esm.writeHNT("INDX", it->mPart);
        esm.writeHNOString("BNAM", it->mMale);
        esm.writeHNOString("CNAM", it->mFemale);
    }
}

unsigned int Armor::sRecordId = REC_ARMO;

void Armor::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    mScript = esm.getHNOString("SCRI");
    esm.getHNT(mData, "AODT", 24);
    mIcon = esm.getHNOString("ITEX");
    mParts.load(esm);
    mEnchant = esm.getHNOString("ENAM");
}

void Armor::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNT("AODT", mData, 24);
    esm.writeHNOCString("ITEX", mIcon);
    mParts.save(esm);
    esm.writeHNOCString("ENAM", mEnchant);
}

    void Armor::blank()
    {
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mHealth = 0;
        mData.mEnchant = 0;
        mData.mArmor = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEnchant.clear();
    }
}
