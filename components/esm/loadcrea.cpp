#include "loadcrea.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM {

void Creature::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mOriginal = esm.getHNOString("CNAM");
    mName = esm.getHNOString("FNAM");
    mScript = esm.getHNOString("SCRI");

    esm.getHNT(mData, "NPDT", 96);

    esm.getHNT(mFlags, "FLAG");
    mScale = 1.0;
    esm.getHNOT(mScale, "XSCL");

    mInventory.load(esm);
    mSpells.load(esm);

    if (esm.isNextSub("AIDT"))
    {
        esm.getHExact(&mAiData, sizeof(mAiData));
        mHasAI = true;
    }
    else
        mHasAI = false;

    mAiPackage.load(esm);
    esm.skipRecord();
}

void Creature::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("CNAM", mOriginal);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNT("NPDT", mData, 96);
    esm.writeHNT("FLAG", mFlags);
    if (mScale != 1.0) {
        esm.writeHNT("XSCL", mScale);
    }

    mInventory.save(esm);
    mSpells.save(esm);
    if (mHasAI) {
        esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
    }
    mAiPackage.save(esm);
}

}
