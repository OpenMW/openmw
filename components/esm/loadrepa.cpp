#include "loadrepa.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Repair::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");

    esm.getHNT(mData, "RIDT", 16);

    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
}

void Repair::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);

    esm.writeHNT("RIDT", mData, 16);
    esm.writeHNOString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}


}
