#include "loadlock.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Lockpick::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");

    esm.getHNT(mData, "LKDT", 16);

    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
}

void Lockpick::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    
    esm.writeHNT("LKDT", mData, 16);
    esm.writeHNOString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}


}
