#include "loadingr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Ingredient::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");
    esm.getHNT(mData, "IRDT", 56);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
    // horrible hack to fix broken data in records
    for (int i=0; i<4; ++i)
    {
        if (mData.mEffectID[i] != 85 &&
            mData.mEffectID[i] != 22 &&
            mData.mEffectID[i] != 17 &&
            mData.mEffectID[i] != 79 &&
            mData.mEffectID[i] != 74)
        {
            mData.mAttributes[i] = -1;
        }
        
        // is this relevant in cycle from 0 to 4?
        if (mData.mEffectID[i] != 89 &&
            mData.mEffectID[i] != 26 &&
            mData.mEffectID[i] != 21 &&
            mData.mEffectID[i] != 83 &&
            mData.mEffectID[i] != 78)
        {
            mData.mSkills[i] = -1;
        }
    }
}

void Ingredient::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    esm.writeHNT("IRDT", mData, 56);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}

}
