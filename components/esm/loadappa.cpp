#include "loadappa.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{
void Apparatus::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");
    esm.getHNT(mData, "AADT", 16);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNString("ITEX");
}
void Apparatus::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    esm.writeHNT("AADT", mData, 16);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNCString("ITEX", mIcon);
}
}
