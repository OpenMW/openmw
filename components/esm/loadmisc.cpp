#include "loadmisc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Miscellaneous::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "MCDT", 12);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
}
void Miscellaneous::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("MCDT", mData, 12);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}

}
