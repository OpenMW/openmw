#include "loadlocks.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Tool::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");

    esm.getSubName();
    NAME n = esm.retSubName();
    // The data name varies, RIDT for repair items, LKDT for lock
    // picks, PBDT for probes

    esm.getHT(mData, 16);

    if (n == "RIDT")
    {
        mType = Type_Repair;
        // Swap t.data.quality and t.data.uses for repair items (sigh)
        float tmp = *((float*) &mData.mUses);
        mData.mUses = *((int*) &mData.mQuality);
        mData.mQuality = tmp;
    }
    else if (n == "LKDT")
        mType = Type_Pick;
    else if (n == "PBDT")
        mType = Type_Probe;

    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
}

void Tool::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    
    std::string typeName;
    switch(mType)
    {
    case Type_Repair: typeName = "RIDT"; break;
    case Type_Pick: typeName = "LKDT"; break;
    case Type_Probe: typeName = "PBDT"; break;
    }
    
    Data write = mData;
    if (mType == Type_Repair)
    {
        float tmp = *((float*) &write.mUses);
        write.mUses = *((int*) &write.mQuality);
        write.mQuality = tmp;
    }

    esm.writeHNT(typeName, write, 16);
    esm.writeHNOString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}


}
