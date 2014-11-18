#include "loadalch.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Potion::sRecordId = REC_ALCH;

void Potion::load(ESMReader &esm)
{
    mModel = esm.getHNOString("MODL");
    mIcon = esm.getHNOString("TEXT"); // not ITEX here for some reason
    mScript = esm.getHNOString("SCRI");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "ALDT", 12);
    mEffects.load(esm);
}
void Potion::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("TEXT", mIcon);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("ALDT", mData, 12);
    mEffects.save(esm);
}

    void Potion::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mAutoCalc = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEffects.mList.clear();
    }
}
