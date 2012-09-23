#include "loadclot.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Clothing::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "CTDT", 12);

    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");

    mParts.load(esm);
    

    mEnchant = esm.getHNOString("ENAM");
}
void Clothing::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("CTDT", mData, 12);

    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
    
    mParts.save(esm);
    
    esm.writeHNOCString("ENAM", mEnchant);
}

}
